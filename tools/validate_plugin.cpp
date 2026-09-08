#include "plugins/PluginValidation.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <cstdio>
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    if(app.arguments().size()!=2) {std::fprintf(stderr,"Usage: AndiyaPluginValidator <plugin-folder>\n");return 2;}
    auto report=PluginValidation::inspect(app.arguments()[1]);
    std::puts(QJsonDocument::fromVariant(report).toJson().constData());
    return report.value("valid").toBool()?0:1;
}
