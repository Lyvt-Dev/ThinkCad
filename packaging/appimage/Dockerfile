FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates curl xz-utils file patchelf cmake ninja-build g++ python3 python3-pip zlib1g-dev libgl1-mesa-dev libegl1 libxkbcommon-x11-0 libxcb-cursor0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-render-util0 libxcb-xinerama0 libxcb-randr0 libxcb-shape0 libxcb-sync1 libxrender1 libxi6 libasound2 libpulse0 libfontconfig1 fonts-dejavu-core && rm -rf /var/lib/apt/lists/*
ENV PIP_CERT=/etc/ssl/certs/ca-certificates.crt
ENV REQUESTS_CA_BUNDLE=/etc/ssl/certs/ca-certificates.crt
ENV SSL_CERT_FILE=/etc/ssl/certs/ca-certificates.crt
RUN --mount=type=secret,id=build_ca if [ -f /run/secrets/build_ca ]; then cp /run/secrets/build_ca /etc/ssl/certs/ca-certificates.crt; fi; python3 -m pip install --no-cache-dir aqtinstall==3.3.0
RUN python3 -m aqt install-qt linux desktop 6.8.3 linux_gcc_64 -m qtmultimedia qtserialport -O /opt/Qt
ENV PATH="/opt/Qt/6.8.3/gcc_64/bin:${PATH}"
ENV CMAKE_PREFIX_PATH=/opt/Qt/6.8.3/gcc_64
ENV LD_LIBRARY_PATH=/opt/Qt/6.8.3/gcc_64/lib
ENV QMAKE=/opt/Qt/6.8.3/gcc_64/bin/qmake
RUN python3 -m pip install --no-cache-dir cmake==3.31.6
RUN apt-get update && apt-get install -y --no-install-recommends libglib2.0-0 libxrandr2 libxfixes3 libxcursor1 libxdamage1 libxtst6 libva2 libva-drm2 libva-x11-2 libvdpau1 libopengl0 libwayland-egl1 libwayland-cursor0 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
CMD ["bash","packaging/appimage/build.sh"]
