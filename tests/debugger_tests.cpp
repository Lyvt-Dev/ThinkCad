#include "gdbmi.hpp"
#include "gdbsession.hpp"
#include "codeinterpreter.hpp"
#include "projectio.hpp"
#include <QtTest>
#include <QFile>
#include <QTemporaryDir>
#include <QStandardPaths>
#ifndef Q_OS_WIN
#include <signal.h>
#include <cerrno>
#endif
class DebuggerTests:public QObject {
    Q_OBJECT
private slots:
    void parseRecords(){
        QString error;auto r=GdbMi::parse("42^done,variables=[{name=\"counter\",value=\"7\"},{name=\"s\",value=\"a\\n\\\"b\"}]",&error);
        QVERIFY2(r.has_value(),qPrintable(error));QCOMPARE(r->token,42);QCOMPARE(r->kind,QString("done"));const auto values=r->values["variables"].toList();QCOMPARE(values.size(),2);QCOMPARE(values[0].toMap()["value"].toString(),QString("7"));
        r=GdbMi::parse("*stopped,reason=\"breakpoint-hit\",frame={file=\"Sketch.ino\",line=\"12\"}");QVERIFY(r);QCOMPARE(r->values["frame"].toMap()["line"].toString(),QString("12"));
        r=GdbMi::parse("~\"\\303\\251\\n\"");QVERIFY(r);QCOMPARE(r->text,QString::fromUtf8("é\n"));
        const QString text=QString::fromUtf8("C:\\sketch \"é\"\n\t");r=GdbMi::parse("@"+GdbMi::quote(text));QVERIFY(r);QCOMPARE(r->text,text);
        r=GdbMi::parse("7^done,stack=[frame={level=\"0\"},frame={level=\"1\"}]");QVERIFY(r);QCOMPARE(r->values["stack"].toList().size(),2);
        QVERIFY(!GdbMi::parse("(gdb)",&error));QVERIFY(error.isEmpty());
    }
    void malformedRecords_data(){QTest::addColumn<QByteArray>("line");QTest::newRow("trailing")<<QByteArray("^done,");QTest::newRow("bare")<<QByteArray("^done,x=unquoted");QTest::newRow("string")<<QByteArray("~\"unterminated");QTest::newRow("tuple")<<QByteArray("^done,x={a=\"b\"");QTest::newRow("duplicate")<<QByteArray("^done,x=\"1\",x=\"2\"");QTest::newRow("token")<<QByteArray("999999999999999999999^done");QTest::newRow("large")<<QByteArray(262145,'x');QTest::newRow("depth")<<(QByteArray("^done,x=")+QByteArray(30,'[')+"\"x\""+QByteArray(30,']'));}
    void malformedRecords(){QFETCH(QByteArray,line);QString error;QVERIFY(!GdbMi::parse(line,&error));QVERIFY(!error.isEmpty());}
    void sketchDebuggingKeepsRpcAndSourceLocations(){
        QString gdb=qEnvironmentVariable("THINKCAD_TEST_GDB");if(gdb.isEmpty())gdb=QStandardPaths::findExecutable("gdb");if(gdb.isEmpty()){QVERIFY2(!qEnvironmentVariableIsSet("THINKCAD_REQUIRE_GDB"),"Required GDB is unavailable");QSKIP("Optional local GDB is unavailable");}
        SketchProject project;project.name="Debug";project.order={"Debug.ino","Functions.ino"};
        project.files["Debug.ino"]="void setup(){\n  pinMode(4,OUTPUT);\n  int measured=digitalRead(5);\n  digitalWrite(4,measured);\n  helper();\n}\nvoid loop(){delay(10); }\n";
        project.files["Functions.ino"]="void helper(){\n  Serial.println(\"DEBUG_OK\");\n}\n";
        QString log;QByteArray protocol;CodeInterpreter runner;connect(&runner,&CodeInterpreter::log,this,[&](const auto& message){log+=message+'\n';});
        connect(&runner,&CodeInterpreter::protocol,this,[&](const auto& line){protocol+=line+'\n';if(line=="READ 5")runner.answer(1);if(line=="SPEED")runner.answer(1000);});
        QSignalSpy halted(runner.debugger(),&GdbSession::halted),evaluated(runner.debugger(),&GdbSession::evaluated);
        runner.configureDebugger(gdb,{{"Debug.ino",4},{"Functions.ino",2}},project.files);
        runner.start(project.combinedSource(),QStringLiteral(THINKCAD_TEST_COMPILER),QCoreApplication::applicationDirPath()+"/sdk",{},project.auxiliaryFiles());
        QTRY_VERIFY2_WITH_TIMEOUT(halted.size()==1,qPrintable(log),45000);QVERIFY2(runner.active(),qPrintable(log));QCOMPARE(QFileInfo(halted[0][0].toString()).fileName(),QString("Debug.ino"));QCOMPARE(halted[0][1].toInt(),4);QVERIFY(protocol.contains("READ 5"));
        runner.debugger()->evaluate("measured");QTRY_COMPARE(evaluated.size(),1);QCOMPARE(evaluated[0][1].toString(),QString("1"));runner.debugger()->resume();
        QTRY_VERIFY2_WITH_TIMEOUT(halted.size()==2,qPrintable(log),10000);QCOMPARE(QFileInfo(halted[1][0].toString()).fileName(),QString("Functions.ino"));QCOMPARE(halted[1][1].toInt(),2);QVERIFY(protocol.contains("WRITE 4 1"));runner.debugger()->resume();QTRY_VERIFY_WITH_TIMEOUT(protocol.contains("SERIAL 44454255475f4f4b"),5000);runner.stop();QVERIFY(!runner.active());QVERIFY(!runner.debugger()->active());
    }
    void actualBreakpointStepVariablesAndStop(){
        QString gdb=qEnvironmentVariable("THINKCAD_TEST_GDB");if(gdb.isEmpty())gdb=QStandardPaths::findExecutable("gdb");if(gdb.isEmpty()){QVERIFY2(!qEnvironmentVariableIsSet("THINKCAD_REQUIRE_GDB"),"Required GDB is unavailable");QSKIP("Optional local GDB is unavailable");}
        QTemporaryDir dir;QVERIFY(dir.isValid());const auto source=dir.filePath("probe.cpp");QFile file(source);QVERIFY(file.open(QIODevice::WriteOnly));file.write("int main(){\n volatile int counter=7;\n counter=counter+2;\n while(true){counter=counter+1;}\n}\n");file.close();
        const auto executable=dir.filePath("probe.exe");QProcess compiler;compiler.start(QStringLiteral(THINKCAD_TEST_COMPILER),{"-g","-O0",source,"-o",executable});QVERIFY(compiler.waitForFinished(15000));QCOMPARE(compiler.exitCode(),0);
        QString failures,transcript;GdbSession session;QSignalSpy ready(&session,&GdbSession::ready),halted(&session,&GdbSession::halted),variables(&session,&GdbSession::variables),evaluated(&session,&GdbSession::evaluated),registers(&session,&GdbSession::registers),running(&session,&GdbSession::running);connect(&session,&GdbSession::log,this,[&](const auto& text){transcript+=text;});connect(&session,&GdbSession::error,this,[&](const auto& text){failures+=text+'\n';});
        session.start(gdb,executable,QProcessEnvironment::systemEnvironment(),{{source,3}});QTRY_VERIFY2_WITH_TIMEOUT(ready.size()==1,qPrintable(failures+transcript),15000);session.run();QTRY_VERIFY2_WITH_TIMEOUT(halted.size()==1,qPrintable(failures+transcript),15000);QVERIFY2(failures.isEmpty(),qPrintable(failures));QCOMPARE(halted[0][1].toInt(),3);QVERIFY(session.stopped());QVERIFY(session.inferiorPid()>0);
        session.inspectVariables();QTRY_COMPARE(variables.size(),1);const auto locals=variables[0][0].toList();bool found=false;for(const auto& local:locals){const auto item=local.toMap();if(item["name"]=="counter"){QCOMPARE(item["value"].toString(),QString("7"));found=true;}}QVERIFY(found);
        session.next();QTRY_COMPARE(halted.size(),2);QCOMPARE(halted[1][1].toInt(),4);session.evaluate("counter");QTRY_COMPARE(evaluated.size(),1);QCOMPARE(evaluated[0][1].toString(),QString("9"));session.inspectRegisters();QTRY_COMPARE(registers.size(),1);QVERIFY(!registers[0][0].toList().isEmpty());session.resume();QTRY_VERIFY(running.size()>=3);const auto child=session.inferiorPid();session.stop();QVERIFY(!session.active());QCOMPARE(session.inferiorPid(),qint64(0));
#ifndef Q_OS_WIN
        errno=0;QVERIFY(::kill(static_cast<pid_t>(child),0)==-1&&errno==ESRCH);
#endif
    }
};
QTEST_GUILESS_MAIN(DebuggerTests)
#include "debugger_tests.moc"
