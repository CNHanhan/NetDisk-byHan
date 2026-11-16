HEADERS += \
    $$PWD/mysqlutils.h \
    $$PWD/../config.h

SOURCES += \
    $$PWD/mysqlutils.cpp

# MySQL 客户端库：Windows 用 libmysql.lib，Linux 用 -lmysqlclient
win32 {
    INCLUDEPATH += "C:\Program Files\MySQL\MySQL Server 8.0\include"
    LIBS += "C:\Program Files\MySQL\MySQL Server 8.0\lib\libmysql.lib"
}

unix {
    INCLUDEPATH += /usr/include/mysql
    LIBS += -lmysqlclient
}
