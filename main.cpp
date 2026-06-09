#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QTime>
#include <QScreen>
#include <QGuiApplication>
#include <QList>
#include <QRandomGenerator>
#include <QObject>

#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

#include <QFile>
#include <QTextStream>

class ClockWindow : public QWidget
{
public:
    ClockWindow(QScreen *screen,
                const QString &title)
    {
        setScreen(screen);

        setStyleSheet("background-color:black;");
        setWindowFlags(Qt::FramelessWindowHint);

        setGeometry(screen->geometry());

        titleLabel = new QLabel(title, this);
        titleLabel->setStyleSheet("color:white;");

        QFont titleFont;
        titleFont.setPointSize(40);
        titleFont.setBold(true);
        titleFont.setLetterSpacing(
            QFont::AbsoluteSpacing,
            4
        );

        titleLabel->setFont(titleFont);
        titleLabel->adjustSize();

        clockLabel = new QLabel(this);
        clockLabel->setStyleSheet("color:white;");

        QFont clockFont;
        clockFont.setPointSize(80);
        clockFont.setBold(true);

        clockLabel->setFont(clockFont);

        updateClock();

        auto *clockTimer = new QTimer(this);

        connect(clockTimer,
                &QTimer::timeout,
                this,
                &ClockWindow::updateClock);

        clockTimer->start(1000);

        titleLabel->hide();
        clockLabel->hide();
    }

    void showContent(bool visible)
    {
        titleLabel->setVisible(visible);
        clockLabel->setVisible(visible);
    }

    void moveToCell(int row, int col)
    {
        int cellW = width() / 3;
        int cellH = height() / 3;

        int centerX =
            col * cellW + cellW / 2;

        int centerY =
            row * cellH + cellH / 2;

        titleLabel->adjustSize();
        clockLabel->adjustSize();

        int titleX =
            centerX - titleLabel->width() / 2;

        int titleY =
            centerY - 90;

        int clockX =
            centerX - clockLabel->width() / 2;

        int clockY =
            centerY - 20;

        titleLabel->move(titleX, titleY);
        clockLabel->move(clockX, clockY);
    }

private:
    QLabel *titleLabel;
    QLabel *clockLabel;

    void updateClock()
    {
        clockLabel->setText(
            QTime::currentTime().toString(
                "HH:mm:ss"
            )
        );

        clockLabel->adjustSize();
    }
};

struct Cell
{
    ClockWindow *window;
    int row;
    int col;
};

class ClockSaver : public QObject
{
public:
    ClockSaver()
    {
        title = "Studio GG";

        QFile file(
            "/opt/gg-clocksaver/gg-clocksaver.conf"
        );

        if (file.open(
                QIODevice::ReadOnly |
                QIODevice::Text))
        {
            QTextStream in(&file);

            QString line =
                in.readLine().trimmed();

            if (!line.isEmpty())
                title = line;
        }

        idleTimer = new QTimer(this);

        connect(idleTimer,
                &QTimer::timeout,
                this,
                &ClockSaver::checkIdle);

        idleTimer->start(1000);

        moveTimer = new QTimer(this);

        connect(moveTimer,
                &QTimer::timeout,
                this,
                &ClockSaver::switchCell);
    }

private:
    QList<ClockWindow*> windows;
    QList<Cell> cells;

    QTimer *idleTimer;
    QTimer *moveTimer;

    bool saverVisible = false;
    QString title;

    unsigned long idleTime()
    {
        Display *display =
            XOpenDisplay(nullptr);

        if (!display)
            return 0;

        XScreenSaverInfo *info =
            XScreenSaverAllocInfo();

        XScreenSaverQueryInfo(
            display,
            DefaultRootWindow(display),
            info
        );

        unsigned long idle =
            info->idle;

        XFree(info);
        XCloseDisplay(display);

        return idle;
    }

    void showSaver()
    {
        if (saverVisible)
            return;

        windows.clear();
        cells.clear();

        QList<QScreen*> screens =
            QGuiApplication::screens();

        for (QScreen *screen : screens)
        {
            auto *window =
                new ClockWindow(
                    screen,
                    title
                );

            window->setCursor(
                Qt::BlankCursor
            );

            window->showFullScreen();

            windows.append(window);

            for (int row = 0; row < 3; ++row)
            {
                for (int col = 0; col < 3; ++col)
                {
                    cells.append(
                    {
                        window,
                        row,
                        col
                    });
                }
            }
        }

        switchCell();

        moveTimer->start(10000);

        saverVisible = true;
    }

    void hideSaver()
    {
        if (!saverVisible)
            return;

        moveTimer->stop();

        for (auto *window : windows)
        {
            window->close();
            delete window;
        }

        windows.clear();
        cells.clear();

        saverVisible = false;
    }

    void switchCell()
    {
        if (cells.isEmpty())
            return;

        for (auto *window : windows)
            window->showContent(false);

        int index =
            QRandomGenerator::global()
            ->bounded(cells.size());

        Cell &cell =
            cells[index];

        cell.window->moveToCell(
            cell.row,
            cell.col
        );

        cell.window->showContent(true);
    }

    void checkIdle()
    {
        const unsigned long timeout =
            240000; // 4 minuta

        unsigned long idle =
            idleTime();

        if (!saverVisible)
        {
            if (idle >= timeout)
                showSaver();
        }
        else
        {
            if (idle < 1000)
                hideSaver();
        }
    }
public:
    void showPreview()
    {
        showSaver();
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setQuitOnLastWindowClosed(false);

    ClockSaver saver;

    QStringList args = app.arguments();

    if (args.contains("--preview"))
    {
        saver.showPreview();
    }

    return app.exec();
}