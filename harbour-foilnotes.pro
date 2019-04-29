TEMPLATE = subdirs
SUBDIRS = app qrencode

app.file = app.pro
app.depends = qrencode-target

qrencode.file = qrencode.pro
qrencode.target = qrencode-target

OTHER_FILES += README.md LICENSE rpm/*.spec
