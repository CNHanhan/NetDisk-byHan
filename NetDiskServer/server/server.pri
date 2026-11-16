HEADERS += \
    $$PWD/INet.h \
    $$PWD/tcpserver.h

SOURCES += \
    $$PWD/tcpserver.cpp

# Windows 需要显式链接 winsock 库，Linux 下 socket 在 libc 中
win32 {
    LIBS += -lws2_32
}

unix {
    LIBS += -lpthread
}
