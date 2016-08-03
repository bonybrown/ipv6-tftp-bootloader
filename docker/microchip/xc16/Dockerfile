FROM i386/debian

ADD http://ww1.microchip.com/downloads/en/DeviceDoc/xc16-v1.26-full-install-linux-installer.run /tmp/

RUN apt-get update \
  && apt-get -y install make srecord \
  && chmod a+x /tmp/xc16-v1.26-full-install-linux-installer.run \
  && sync \
  && /tmp/xc16-v1.26-full-install-linux-installer.run --mode unattended --netservername none \
  && rm -rf /tmp/* \
  && rm -rf /var/lib/apt/lists/*
  
VOLUME /data

WORKDIR /data


 
