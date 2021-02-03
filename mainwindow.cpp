#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

#define AUDIBLE_RANGE_START 20
#define AUDIBLE_RANGE_END   20000 /* very optimistic */
#define NUM_SAMPLES 96000
#define SAMPLE_FREQ 48000

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Sound");


    QList<QAudioDeviceInfo> inputDevices =
            QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    for (QAudioDeviceInfo d : inputDevices) {
        ui->inputDeviceComboBox->addItem(d.deviceName(),QVariant::fromValue(d));
    }

    /* Setup plot */
    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->plot->legend->setVisible(true);
    QFont legendFont = font();
    legendFont.setPointSize(10);
    ui->plot->legend->setFont(legendFont);
    ui->plot->legend->setSelectedFont(legendFont);
    ui->plot->legend->setSelectableParts(QCPLegend::spItems);
    ui->plot->yAxis->setLabel("Amplitude");
    ui->plot->xAxis->setLabel("Sample");
    ui->plot->yAxis->setRange(-1.0, 1.0);
    ui->plot->clearGraphs();
    ui->plot->addGraph();

    ui->plot->graph()->setPen(QPen(Qt::black));
    ui->plot->graph()->setName("Audio In");

    /* Setup plot 2 */
    ui->plot2->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->plot2->legend->setVisible(false);
    ui->plot2->yAxis->setLabel("");
    ui->plot2->xAxis->setLabel("Frequency");
    ui->plot2->xAxis->setRange(AUDIBLE_RANGE_START, AUDIBLE_RANGE_END);
    ui->plot2->yAxis->setRange(0.0, 500.0);
    ui->plot2->clearGraphs();
    ui->plot2->addGraph();

    ui->plot2->graph()->setPen(QPen(Qt::black));
    ui->plot2->graph()->setName("fft");


    for (int i = 0; i < NUM_SAMPLES; i ++) {
        mIndices.append((double)i);
        mSamples.append(0);
    }

    double freqStep = (double)SAMPLE_FREQ / (double)NUM_SAMPLES;
    double f = AUDIBLE_RANGE_START;
    while (f < AUDIBLE_RANGE_END) {
        mFftIndices.append(f);
        f += freqStep;
    }

    /* Set up FFT plan */
    mFftIn  = fftw_alloc_real(NUM_SAMPLES);
    mFftOut = fftw_alloc_real(NUM_SAMPLES);
    mFftPlan = fftw_plan_r2r_1d(NUM_SAMPLES, mFftIn, mFftOut, FFTW_R2HC,FFTW_ESTIMATE);
}

MainWindow::~MainWindow()
{
    delete ui;

    fftw_free(mFftIn);
    fftw_free(mFftOut);
    fftw_destroy_plan(mFftPlan);
}


void MainWindow::on_refreshInputPushButton_clicked()
{
    QList<QAudioDeviceInfo> inputDevices =
            QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    ui->inputDeviceComboBox->clear();
    for (QAudioDeviceInfo d : inputDevices) {
        ui->inputDeviceComboBox->addItem(d.deviceName(),QVariant::fromValue(d));
    }
}

void MainWindow::on_useInputPushButton_clicked()
{
    QVariant v = ui->inputDeviceComboBox->currentData();
    QAudioDeviceInfo dev = v.value<QAudioDeviceInfo>();

    QAudioFormat format;
    format.setSampleRate(SAMPLE_FREQ);
    format.setChannelCount(1);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setCodec("raw");
    format.setSampleSize(16);

    if (mAudioIn) delete mAudioIn;
    mAudioIn = nullptr;

    mAudioIn = new QAudioInput(dev,format);
    mAudioIn->setVolume(0.1);
    mAudioIn->setNotifyInterval(100);

    connect(mAudioIn, &QAudioInput::notify,
            this, &MainWindow::processAudioIn);
    connect(mAudioIn, &QAudioInput::stateChanged,
            this, &MainWindow::stateChangeAudioIn);

    mInputBuffer.open(QBuffer::ReadWrite);
    mAudioIn->start(&mInputBuffer);
}

void MainWindow::processAudioIn()
{
    mInputBuffer.seek(0);
    QByteArray ba = mInputBuffer.readAll();

    int num_samples = ba.length() / 2;
    int b_pos = 0;
    for (int i = 0; i < num_samples; i ++) {
        int16_t s;
        s = ba.at(b_pos++);
        s |= ba.at(b_pos++) << 8;
        if (s != 0) {
            mSamples.append((double)s / 32768.0);
        } else {
            mSamples.append(0);
        }
    }
    mInputBuffer.buffer().clear();
    mInputBuffer.seek(0);

    samplesUpdated();
}

void MainWindow::stateChangeAudioIn(QAudio::State s)
{
    qDebug() << "State change: " << s;
}

void MainWindow::on_volumeHorizontalSlider_sliderMoved(int position)
{
    if (mAudioIn) {
        mAudioIn->setVolume((double)position/1000.0);
    }
}

void MainWindow::samplesUpdated()
{
    int n = mSamples.length();
    if (n > 96000) mSamples = mSamples.mid(n - NUM_SAMPLES,-1);

    memcpy(mFftIn, mSamples.data(), NUM_SAMPLES*sizeof(double));

    fftw_execute(mFftPlan);

    QVector<double> fftVec;

    for (int i = (NUM_SAMPLES/SAMPLE_FREQ)*AUDIBLE_RANGE_START;
             i < (NUM_SAMPLES/SAMPLE_FREQ)*AUDIBLE_RANGE_END;
             i ++) {
        fftVec.append(abs(mFftOut[i]));
    }

    ui->plot->graph(0)->setData(mIndices,mSamples);
    ui->plot->xAxis->rescale();
    ui->plot->replot();

    ui->plot2->graph(0)->setData(mFftIndices.mid(0,fftVec.length()),fftVec);
    ui->plot2->replot();
}
