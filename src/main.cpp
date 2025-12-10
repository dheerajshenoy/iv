#include "MainWindow.hpp"
#include "argparse.hpp"

int
main(int argc, char *argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    MainWindow mw;

    argparse::ArgumentParser program("iv", __IV_VERSION);
    program.add_argument("files").remaining().metavar("FILE_PATH(s)");
    program.add_argument("version").flag().help("Show version information");

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::exception &e)
    {
        qDebug() << e.what();
    }

    mw.readArgs(program);
    app.exec();
}
