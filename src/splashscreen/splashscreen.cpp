#include "splashscreen.h"

#include <QApplication>
#include <QScreen>
#include <QThread>

SplashScreen::SplashScreen()
{
    QPixmap pic(":/img/SPLASHSCREEN.png");

    // get screen size
    QSize size = qApp->screens().at(0)->size();

    setPixmap(pic.scaled(size.width() / 2,
                         static_cast<int>(size.height() * 0.26),
                         Qt::KeepAspectRatio));

    showMessage("Group LV-425.C++",
                Qt::AlignRight | Qt::AlignBottom,
                Qt::red);
}

void SplashScreen::start()
{
    show();
    QThread::sleep(1);
}
