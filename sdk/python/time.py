"""Virtual board time; delays honor ThinkCad speed, pause and stepping."""
import machine as _machine

def sleep(seconds): _machine._sleep(int(seconds * 1000000))
def sleep_ms(ms): _machine._sleep(int(ms) * 1000)
def sleep_us(us): _machine._sleep(us)
def ticks_us(): return _machine._clock & 0x3fffffff
def ticks_ms(): return (_machine._clock // 1000) & 0x3fffffff
def ticks_add(ticks, delta): return (ticks + delta) & 0x3fffffff
def ticks_diff(a, b): return ((a - b + 0x20000000) & 0x3fffffff) - 0x20000000
