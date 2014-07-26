//
//  sinesweep.c
//  
//
//  Created by meng tony on 13-10-9.
//
//

#include <stdio.h>
#include <math.h>
#include "portaudio.h"

#define NUM_SECONDS (5)
#define SAMPLE_RATE (44100.0)
#define FRAMES_PER_BUFFER (128)
#define start_freq  (20.0)
#define end_freq    (20000.0)
#define gain (0.5)

#ifndef TWO_PI
#define TWO_PI    (6.28318530717959)
#endif

#define TABLE_SIZE  (256)
typedef struct
{
    float   freq_sweep;
    float   left_phase;
    float   right_phase;
    float   inc_freq;
}
paData;

/* This routine will be called by the PortAudio engine when audio is needed.
 ** It may called at interrupt level on some machines so don't do anything
 ** that could mess up the system like calling malloc() or free().
 */
static int patestCallback( const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData )
{
    paData *data = (paData*)userData;   
    float *out = (float*)outputBuffer;
    unsigned long i;
    
    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;
    
    for( i=0; i<framesPerBuffer; i++ )
    {
        *out++ = gain * sin(data->left_phase);  /* left */
        *out++ = gain * sin(data->right_phase);  /* right */
        data->left_phase += TWO_PI * (pow(10,data->freq_sweep) / SAMPLE_RATE);
        if( data->left_phase >= TWO_PI ) data->left_phase -= TWO_PI;
        data->right_phase += TWO_PI * (pow(10,data->freq_sweep) / SAMPLE_RATE);
        if( data->right_phase >= TWO_PI ) data->right_phase -= TWO_PI;
        data->freq_sweep += data->inc_freq;
        if(data->freq_sweep >= log10(end_freq)) data->freq_sweep = log10(start_freq);
    }
    
    return paContinue;
}

/*
 * This routine is called by portaudio when playback is done.
 */
static void StreamFinished( void* userData )
{
    paData *data = (paData *) userData;
    /* printf( "Stream Completed: %s\n", data->message ); */
}

int main(void)
{
    PaError err;
    PaStreamParameters outputParameters;
    PaStream *stream;
    paData data;
    int i;
    float start_log_freq;
    float end_log_freq;
    const PaStreamInfo *streamInfo;
    
    printf("PortAudio Test: output sine wave. SR = %f, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);
    
    /* Initialise userData */
    start_log_freq = log10(start_freq);
    end_log_freq = log10(end_freq);
    
    data.left_phase = data.right_phase = 0;
    data.freq_sweep = start_log_freq;
    data.inc_freq = (end_log_freq - start_log_freq)/(NUM_SECONDS * SAMPLE_RATE);
    
    err = Pa_Initialize();
    if(err != paNoError) goto error;
    
    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
        fprintf(stderr,"Error: No default output device.\n");
        goto error;
    }
    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
                        &stream,
                        NULL, /* no input */
                        &outputParameters,
                        SAMPLE_RATE,
                        FRAMES_PER_BUFFER,
                        paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                        patestCallback,
                        &data );
    if( err != paNoError ) goto error;
    
    /*sprintf( data.message, "No Message" ); */
    err = Pa_SetStreamFinishedCallback( stream, &StreamFinished );
    if( err != paNoError ) goto error;
    
    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;
    
    streamInfo = Pa_GetStreamInfo(stream);
    printf("outputLatency is %f Second\n",streamInfo->outputLatency);
    printf("inputLatency is %f Second\n",streamInfo->inputLatency);

    
    printf("Hit ENTER to stop program.\n");
    getchar();
    
    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;
    
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;
    
    Pa_Terminate();
    printf("Test finished.\n");
    
    return err;
error:
    /* Pa_Terminate(); */
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;


}