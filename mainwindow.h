#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtMultimedia/QAudioInput>
#include <QBuffer>
#include "qcustomplot.h"
#include <fftw3.h>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();    

private slots:
    void on_refreshInputPushButton_clicked();
    void on_useInputPushButton_clicked();

    void processAudioIn();
    void stateChangeAudioIn(QAudio::State s);
    void on_volumeHorizontalSlider_sliderMoved(int position);

private:
    Ui::MainWindow *ui;

    void samplesUpdated();

    QList<QAudioDeviceInfo> mInputDevices;
    QAudioInput *mAudioIn = nullptr;
    QBuffer  mInputBuffer;

    QVector<double> mSamples;
    QVector<double> mIndices;
    QVector<double> mFftIndices;

    fftw_plan mFftPlan;
    double *mFftIn;
    double *mFftOut;


};
#endif // MAINWINDOW_H
