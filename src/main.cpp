#include "MainWindow.hpp"
#include "argparse.hpp"

int
main(int argc, char *argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    MainWindow mw;

    argparse::ArgumentParser program("iv", __IV_VERSION);
    // program.add_argument("version").flag().help("Show version information");
    // program.add_argument("commands").flag().help("Show list of available commands, useful for assigning shortcuts");
    // program.add_argument("files").remaining().metavar("FILE_PATH(s)");

    // Add version, commands, and files arguments
    program.add_argument("-v", "--version")
        .help("Show version information")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-c", "--commands")
        .help("Show list of available commands, useful for assigning shortcuts")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("files")
        .help("File path(s) to open")
        .remaining()
        .metavar("FILE_PATH(s)");

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
