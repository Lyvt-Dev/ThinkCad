"""ThinkCad MicroPython host adapters for the modeled board peripherals.

This runs on real MicroPython, in a separate process. It is not ESP firmware.
Unmodeled features raise errors instead of silently succeeding.
"""
from _thinkcad import emit as _emit, request as _request, read as _read, sleep_us as _host_sleep
from _thinkcad_config import config as _config

_clock = 0
_handlers = {}
_in_irq = False

def _poll():
    global _in_irq
    if not _handlers or _in_irq:
        return
    count = _request('IRQ_POLL')
    if not 0 <= count <= 64:
        raise RuntimeError('Invalid interrupt response')
    pending = [_read() for _ in range(count)]
    _in_irq = True
    try:
        for number in pending:
            if number in _handlers:
                handler, pin = _handlers[number]
                handler(pin)
    finally:
        _in_irq = False

def _sleep(us):
    global _clock
    us = int(us)
    if us < 0 or _clock + us > 0xffffffffffffffff:
        raise ValueError('Invalid simulation delay')
    while True:
        speed = _request('SPEED')
        if not 100 <= speed <= 10000:
            raise RuntimeError('Invalid simulation speed')
        step = min(us, 1000 if _handlers and _config['emulation'] else 50 * speed)
        _host_sleep(step * 1000 // speed)
        _clock += step
        us -= step
        _emit('TIME ' + str(_clock))
        _poll()
        if not us:
            break

def _number(pin):
    if isinstance(pin, Pin):
        return pin.id
    if isinstance(pin, str):
        pin = _config['aliases'].get(pin, -1)
    if pin not in _config['gpio']:
        raise ValueError('Unavailable GPIO on ' + _config['board'])
    return pin

class Pin:
    IN = 0
    OUT = 1
    PULL_UP = 1
    PULL_DOWN = 2
    IRQ_RISING = 1
    IRQ_FALLING = 2

    def __init__(self, id, mode=None, pull=None, *, value=None, drive=None, alt=None):
        self.id = _number(id)
        if mode is not None or pull is not None or value is not None:
            self.init(self.IN if mode is None else mode, pull, value=value, drive=drive, alt=alt)
        elif drive is not None or alt is not None:
            raise NotImplementedError('Pin drive strength and alternate functions are not modeled')

    def init(self, mode=IN, pull=None, *, value=None, drive=None, alt=None):
        if drive is not None or alt is not None or pull not in (None, self.PULL_UP):
            raise NotImplementedError('Only GPIO input, output and pull-up are modeled')
        if mode not in (self.IN, self.OUT) or (pull is not None and mode != self.IN):
            raise ValueError('Invalid pin mode')
        if (mode == self.OUT or pull is not None) and self.id in _config['input_only']:
            raise ValueError('This GPIO is input-only')
        if self.id == 16 and pull is not None and _config['board'] == 'esp8266':
            raise ValueError('ESP8266 GPIO16 has no modeled pull-up')
        self._mode = mode
        _emit('MODE %d %d' % (self.id, 2 if pull is not None else mode))
        if value is not None:
            self.value(value)
        _poll()

    def value(self, value=None):
        if value is None:
            result = _request('READ ' + str(self.id))
            if result not in (0, 1):
                raise RuntimeError('GPIO is floating or has conflicting drivers')
            _poll()
            return result
        _emit('WRITE %d %d' % (self.id, int(bool(value))))
        _poll()

    __call__ = value
    def on(self): self.value(1)
    def off(self): self.value(0)
    def toggle(self): self.value(not self.value())
    def irq(self, handler=None, trigger=IRQ_RISING | IRQ_FALLING, *, hard=False):
        if hard:
            raise NotImplementedError('Only soft GPIO interrupts are modeled')
        if handler is None:
            _request('IRQ_DETACH ' + str(self.id))
            _handlers.pop(self.id, None)
            return
        if not callable(handler) or trigger not in (1, 2, 3):
            raise ValueError('Invalid GPIO interrupt')
        _request('IRQ_ATTACH %d %d' % (self.id, {1: 3, 2: 2, 3: 1}[trigger]))
        _handlers[self.id] = (handler, self)

class PWM:
    def __init__(self, pin, *, freq=1000, duty_u16=0, duty=None):
        self.pin = Pin(pin, Pin.OUT)
        self._freq = 1000
        self._duty = 0
        self.freq(freq)
        if duty is None: self.duty_u16(duty_u16)
        else: self.duty(duty)
    def _apply(self): _emit('PWM %d %d 65535 %d' % (self.pin.id, self._duty, self._freq))
    def freq(self, value=None):
        if value is None: return self._freq
        if not isinstance(value, int) or not 1 <= value <= 20000: raise ValueError('PWM frequency must be 1..20000 Hz')
        self._freq = value
        self._apply()
    def duty_u16(self, value=None):
        if value is None: return self._duty
        if not isinstance(value, int) or not 0 <= value <= 65535: raise ValueError('PWM duty must be 0..65535')
        self._duty = value
        self._apply()
    def duty(self, value=None):
        if value is None: return self._duty * 1023 // 65535
        if not isinstance(value, int) or not 0 <= value <= 1023: raise ValueError('PWM duty must be 0..1023')
        self.duty_u16((value * 65535 + 511) // 1023)
    def deinit(self):
        _emit('NOTONE ' + str(self.pin.id))
        self.pin.init(Pin.IN)

class ADC:
    ATTN_11DB = 3
    WIDTH_12BIT = 12
    def __init__(self, pin):
        self.id = pin.id if isinstance(pin, Pin) else pin
        if _config['board'] == 'esp8266' and self.id == 0: self.id = 17
        if self.id not in _config['adc']: raise ValueError('Unavailable ADC input')
    def read(self): return _request('ADC ' + str(self.id))
    def read_u16(self): return self.read() * 65535 // ((1 << _config['adc_bits']) - 1)
    def read_uv(self): return int(self.read() * _config['adc_max'] * 1000000 / ((1 << _config['adc_bits']) - 1))
    def atten(self, value):
        if value != self.ATTN_11DB or _config['adc_max'] != 3.3: raise NotImplementedError('ADC uses the board profile voltage range')
    def width(self, value):
        if value != _config['adc_bits']: raise NotImplementedError('ADC uses the board profile resolution')

def _hex(data):
    alphabet = '0123456789abcdef'
    return ''.join(alphabet[b >> 4] + alphabet[b & 15] for b in data) or '-'

class I2C:
    def __init__(self, id=0, *, scl=None, sda=None, freq=400000, timeout=50000):
        if id not in (0, -1): raise NotImplementedError('Only the primary I2C controller is modeled')
        if not isinstance(freq, int) or not 1 <= freq <= 400000: raise ValueError('I2C frequency must be 1..400000 Hz')
        self.scl = _number(_config['scl'] if scl is None else scl)
        self.sda = _number(_config['sda'] if sda is None else sda)
        if self.scl == self.sda: raise ValueError('SDA and SCL must differ')
        self.freq = freq
        _emit('I2C %d %d' % (self.sda, self.scl))
    def _address(self, address):
        if not isinstance(address, int) or not 0 <= address <= 127: raise ValueError('Invalid I2C address')
    def _advance(self, count):
        if _config['emulation']: _sleep(((count + 1) * 9000000 + self.freq - 1) // self.freq)
    def scan(self):
        found = []
        for a in range(8, 120):
            if _request('I2C_WRITE %d 1 %d -' % (a, self.freq)) == 0: found.append(a)
            self._advance(0)
        return found
    def writeto(self, address, buf, stop=True):
        self._address(address)
        data = bytes(buf)
        if len(data) > 128: raise ValueError('I2C transaction exceeds 128-byte modeled buffer')
        status = _request('I2C_WRITE %d %d %d %s' % (address, bool(stop), self.freq, _hex(data)))
        if status: raise OSError(5)
        self._advance(len(data))
        return len(data)
    def readfrom(self, address, nbytes, stop=True):
        self._address(address)
        if not isinstance(nbytes, int) or not 0 <= nbytes <= 128: raise ValueError('I2C read must be 0..128 bytes')
        count = _request('I2C_READ %d %d %d %d' % (address, nbytes, bool(stop), self.freq))
        if not 0 <= count <= nbytes: raise RuntimeError('Invalid I2C response')
        data = bytes(_read() for _ in range(count))
        if count != nbytes: raise OSError(5)
        self._advance(count)
        return data
    def readfrom_into(self, address, buf, stop=True): buf[:] = self.readfrom(address, len(buf), stop)
    def _prefix(self, address, size):
        if size not in (8, 16) or not 0 <= address < (1 << size): raise ValueError('Invalid I2C memory address')
        return bytes((address >> shift) & 255 for shift in range(size - 8, -1, -8))
    def writeto_mem(self, address, memaddr, buf, *, addrsize=8): self.writeto(address, self._prefix(memaddr, addrsize) + bytes(buf))
    def readfrom_mem(self, address, memaddr, nbytes, *, addrsize=8):
        self.writeto(address, self._prefix(memaddr, addrsize), False)
        return self.readfrom(address, nbytes)
    def readfrom_mem_into(self, address, memaddr, buf, *, addrsize=8): buf[:] = self.readfrom_mem(address, memaddr, len(buf), addrsize=addrsize)
    def deinit(self): _emit('I2C_END')

SoftI2C = I2C

class SPI:
    MSB = 1
    LSB = 0
    def __init__(self, id=1, *, baudrate=1000000, polarity=0, phase=0, bits=8, firstbit=MSB, sck=None, mosi=None, miso=None):
        if id not in (0, 1, 2) or bits != 8: raise NotImplementedError('Only byte-oriented SPI is modeled')
        if polarity not in (0, 1) or phase not in (0, 1) or firstbit not in (0, 1): raise ValueError('Invalid SPI mode')
        if not isinstance(baudrate, int) or not 1 <= baudrate <= 20000000: raise ValueError('Invalid SPI clock')
        defaults = _config['spi']
        self.sck = _number(defaults[0] if sck is None else sck)
        self.miso = _number(defaults[1] if miso is None else miso)
        self.mosi = _number(defaults[2] if mosi is None else mosi)
        if len(set((self.sck, self.miso, self.mosi))) != 3: raise ValueError('SPI pins must differ')
        self.hz, self.mode, self.order = baudrate, polarity * 2 + phase, firstbit
        Pin(self.sck, Pin.OUT)
        Pin(self.miso, Pin.IN)
        Pin(self.mosi, Pin.OUT)
    def _transfer(self, value):
        result = _request('SPI_BYTE %d %d %d %d %d %d %d 0' % (self.sck, self.miso, self.mosi, self.hz, self.mode, self.order, value))
        if not 0 <= result <= 255: raise RuntimeError('Invalid SPI response')
        if _config['emulation']: _sleep((8000000 + self.hz - 1) // self.hz)
        return result
    def write(self, buf):
        if len(buf) > 4096: raise ValueError('SPI transfer exceeds 4096 bytes')
        for b in buf: self._transfer(b)
    def read(self, nbytes, write=0):
        if not isinstance(nbytes, int) or not 0 <= nbytes <= 4096 or not 0 <= write <= 255: raise ValueError('Invalid SPI read')
        return bytes(self._transfer(write) for _ in range(nbytes))
    def readinto(self, buf, write=0): buf[:] = self.read(len(buf), write)
    def write_readinto(self, write_buf, read_buf):
        if len(write_buf) != len(read_buf) or len(write_buf) > 4096: raise ValueError('SPI buffers must have equal bounded lengths')
        for i, value in enumerate(write_buf): read_buf[i] = self._transfer(value)
    def deinit(self):
        Pin(self.sck, Pin.IN)
        Pin(self.mosi, Pin.IN)

SoftSPI = SPI

class UART:
    def __init__(self, id, baudrate=9600, *, bits=8, parity=None, stop=1, tx=None, rx=None, timeout=0, timeout_char=0):
        self.id = id
        self.init(baudrate, bits=bits, parity=parity, stop=stop, tx=tx, rx=rx, timeout=timeout, timeout_char=timeout_char)
    def init(self, baudrate=9600, *, bits=8, parity=None, stop=1, tx=None, rx=None, timeout=0, timeout_char=0):
        if bits not in (5, 6, 7, 8) or parity not in (None, 0, 1) or stop not in (1, 2): raise ValueError('Invalid UART format')
        if not isinstance(baudrate, int) or not 300 <= baudrate <= 2000000 or not 0 <= timeout <= 60000 or not 0 <= timeout_char <= 60000: raise ValueError('Invalid UART baud rate or timeout')
        format = ((bits - 5) << 1) | (8 if stop == 2 else 0) | (0 if parity is None else 0x20 if parity == 0 else 0x30)
        result = _request('UART_BEGIN %d %d %d %d %d 0' % (self.id, baudrate, format, -1 if rx is None else _number(rx), -1 if tx is None else _number(tx)))
        self.rx, self.tx = _read(), _read()
        if result != 1: raise OSError(5)
        self.baudrate, self.frame_bits, self.timeout, self.timeout_char = baudrate, 1 + bits + stop + int(parity is not None), timeout, timeout_char
    def any(self): return _request('UART_AVAILABLE ' + str(self.id))
    def read(self, nbytes=None):
        if nbytes is None: nbytes = self.any()
        if not isinstance(nbytes, int) or not 0 <= nbytes <= 4096: raise ValueError('UART read must be 0..4096 bytes')
        data = bytearray()
        deadline = _clock + self.timeout * 1000
        while len(data) < nbytes:
            value = _request('UART_READ ' + str(self.id))
            if value >= 0:
                data.append(value)
                deadline = _clock + self.timeout_char * 1000
            elif _clock < deadline: _sleep(min(1000, deadline - _clock))
            else: break
        return bytes(data) if data else None
    def readinto(self, buf, nbytes=None):
        data = self.read(len(buf) if nbytes is None else min(nbytes, len(buf)))
        if data is None: return None
        buf[:len(data)] = data
        return len(data)
    def readline(self):
        data = bytearray()
        while len(data) < 4096:
            value = self.read(1)
            if value is None: break
            data.extend(value)
            if value == b'\n': break
        return bytes(data) if data else None
    def write(self, buf):
        data = buf.encode() if isinstance(buf, str) else bytes(buf)
        if len(data) > 4096: raise ValueError('UART write exceeds 4096 bytes')
        sent = 0
        for offset in range(0, len(data), 512):
            chunk = data[offset:offset + 512]
            count = _request('UART_WRITE %d %s' % (self.id, _hex(chunk)))
            if not 0 <= count <= len(chunk): raise RuntimeError('Invalid UART response')
            sent += count
            if _config['emulation']: _sleep((self.frame_bits * 1000000 * count + self.baudrate - 1) // self.baudrate)
            if count < len(chunk): break
        return sent
    def deinit(self): _request('UART_END ' + str(self.id))

def idle(): _sleep(0)
def reset(): raise NotImplementedError('Use ThinkCad Reset to restart the sketch')
