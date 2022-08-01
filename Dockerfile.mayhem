FROM --platform=linux/amd64 ubuntu:22.04 as builder

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential software-properties-common git libqt5x11extras5-dev  libqt5widgets5 qtbase5-private-dev libkf5authcore5

RUN add-apt-repository ppa:kubuntu-ppa/backports
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y libkf5threadweaver-dev libkf5i18n-dev libkf5configwidgets-dev \
    libkf5coreaddons-dev libkf5itemviews-dev libkf5itemmodels-dev libkf5kio-dev libkf5parts-dev \
    libkf5solid-dev libkf5windowsystem-dev libkf5notifications-dev libkf5iconthemes-dev libelf-dev \
    libdw-dev cmake extra-cmake-modules gettext libqt5svg5-dev
RUN git clone https://github.com/KDAB/KDDockWidgets ; cd KDDockWidgets ; mkdir build ; cd build ; cmake -DCMAKE_INSTALL_PREFIX=/usr/ .. ; make ; make install

COPY . /repo
WORKDIR /repo/build
RUN cmake ..
RUN make -j8

FROM ubuntu:22.04 as package
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y libdw-dev
COPY --from=builder /repo/build/tests/integrationtests/elfwalk /
