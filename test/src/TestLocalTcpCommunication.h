#ifndef TESTLOCALTCPCOMMUNICATION_H
#define TESTLOCALTCPCOMMUNICATION_H

// Qt
#include <QtTest/QtTest>

// Local
class ServiceTestObject;
class CuteIPCInterface;

class TestLocalTcpCommunication : public QObject
{
  Q_OBJECT

  static const int QBYTEARRAY_SIZE_FOR_BENCHMARK = 200 * 1024 * 1024;
  static const int QIMAGE_HEIGHT_WIDTH_FOR_BENCHMARK = 8000;

  private slots:
    // init, cleanup
    void init();
    void cleanup();

    // basic communications tests
    void testServerStop();
    void testReconnect();

    // basic data transfer tests
    void testDirectCalls();
    void testRemoteSignals();
    void testRemoteSignalsWithSyncCall();
    void testLocalSignals();
    void testRemoteSignalToSignal();

    // benchmark tests
    void benchmarkQByteArrayTransfer();
    void benchmarkQImageTransfer();

    // image tests
    void testImageDPI();
    void testGrayScaleQImageTransfer();

    // 'difficult' connection cases
    void testMultipleObjectsConnection();
    void testMultipleClients();
    void testSignalAfterReturnCall();
//    void testCallIntoInvokedRemoteSignal();

    void testRemoteSignalToMultipleSlots();
    void testLocalSignalToMultipleSlots();

    void testOwnersOnTheServerSide();

    void connectTime();

    // thread tests
    void testThread();
    void testSimultaneousCalls();

  public slots:
    void specialSlot(int);

  signals:
    void specialSlotFinished();

  private:
    ServiceTestObject* m_service;
    CuteIPCInterface* m_interface;

    void sleep(int);
};

#endif //TESTLOCALTCPCOMMUNICATION_H
