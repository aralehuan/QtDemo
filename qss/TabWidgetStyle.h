#ifndef TABWIDGETSTYLE_H
#define TABWIDGETSTYLE_H
const char* TableWidgetStyle = " \
QTabWidget \
{ \
        border:1px solid #FF0000; \
        background:rgba(0, 0, 0,100%); \
        color:white \
} \
QTabWidget::pane \
{ \
    border:none; \
} \
QTabWidget::tab-bar \
{ \
        alignment:left; \
} \
QTabBar::tab \
{ \
    background:transparent; \
    border:1px solid #FF0000; \
    color:white; \
    min-width:48px; \
    min-height:24px; \
} \
QTabBar::tab:hover \
{ \
    background:rgb(255, 255, 255, 100); \
} \
QTabBar::tab:selected \
{ \
    background:red; \
} ";
#endif // TABWIDGETSTYLE_H
