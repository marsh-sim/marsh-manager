#include "appearanceutils.h"

#include <QApplication>
#include <QPalette>

void AppearanceUtils::fixWindowsPalette(QApplication *app)
{
    // this breaks message boxes on Linux
    app->setStyle("fusion");

    QPalette palette = app->palette();

    // light palette seems to work correctly;
    // window color is consistently set correctly, unlike text
    if (palette.color(QPalette::Window).lightness() > 0.5)
        return;

    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(15, 15, 15));
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Highlight, QColor(142, 45, 197).lighter());
    palette.setColor(QPalette::HighlightedText, Qt::black);

    app->setPalette(palette);
}
