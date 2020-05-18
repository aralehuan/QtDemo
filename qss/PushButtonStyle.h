#ifndef PUSHBUTTONSTYLE_H
#define PUSHBUTTONSTYLE_H

const char* PushButtonStyle =" \
QPushButton \
{ \
        border:1px solid #FF0000; \
        background:rgba(0, 0, 0,100%); \
        color:white \
} \
QPushButton:hover \
{ \
        border:1px solid #FFFFFF; \
} \
QPushButton:pressed \
{ \
        border:1px solid #FF0000; border-style: inset; \
}";
#endif // PUSHBUTTONSTYLE_H
