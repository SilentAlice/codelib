#include <stdio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <CoreFoundation/CoreFoundation.h>

static const int kNumberBuffers = 3;
// Create a data structure to manage information needed by the audio queue
struct myAQStruct {
    AudioFileID                     mAudioFile;
    AudioStreamBasicDescription     mDataFormat;
    AudioQueueRef                   mQueue;
    AudioQueueBufferRef             mBuffers[kNumberBuffers];
    SInt64                          mCurrentPacket;
    UInt32                          mNumPacketsToRead;
    AudioStreamPacketDescription    *mPacketDescs;
    bool                            mDone;
};

static struct myAQStruct myInfo;
/* FIXME 441 is too small for some files */
static UInt32 bufferSizeInSamples = 441;
static UInt32 currentPacket;


// Define a playback audio queue callback function
static void AQTestBufferCallback(
    void                   *inUserData,
    AudioQueueRef          inAQ,
    AudioQueueBufferRef    inCompleteAQBuffer
)
{
    struct myAQStruct *myInfo = (struct myAQStruct *)inUserData;
    if (myInfo->mDone) return;
    UInt32 numBytes;
    UInt32 nPackets = myInfo->mNumPacketsToRead;
 
	printf("called\n");
    UInt32 bytesRead = bufferSizeInSamples * 4;
    UInt32 packetsRead = bufferSizeInSamples;

    AudioFileReadPacketData(
			myInfo->mAudioFile,
			false,
			&bytesRead,
			NULL,
			currentPacket,
			&packetsRead,
			inCompleteAQBuffer->mAudioData);
    inCompleteAQBuffer->mAudioDataByteSize = bytesRead;
    currentPacket += packetsRead;

    if (bytesRead == 0) {
        AudioQueueStop(inAQ, false);
    } else {
        AudioQueueEnqueueBuffer(inAQ, inCompleteAQBuffer, 0, NULL);
    }
    /* printf("called\n"); */
    /* AudioFileReadPacketData( */
        /* myInfo->mAudioFile, */
        /* false, */
        /* &numBytes, */
        /* myInfo->mPacketDescs, */
        /* myInfo->mCurrentPacket, */
        /* &nPackets, */
        /* inCompleteAQBuffer->mAudioData */
    /* ); */
    /* printf("read %d packets %d bytes\n", numBytes, nPackets); */
    /* if (nPackets > 0) { */
        /* inCompleteAQBuffer->mAudioDataByteSize = numBytes; */
        /* AudioQueueEnqueueBuffer ( */
            /* inAQ, */
            /* inCompleteAQBuffer, */
            /* (myInfo->mPacketDescs ? nPackets : 0), */
            /* myInfo->mPacketDescs */
        /* ); */
        /* myInfo->mCurrentPacket += nPackets; */
    /* } else { */
        /* AudioQueueStop ( */
            /* myInfo->mQueue, */
            /* false */
        /* ); */
        /* myInfo->mDone = true; */
    /* } */
}

void AudioEnginePropertyListenerProc (void *inUserData, AudioQueueRef inAQ, AudioQueuePropertyID inID) {
    //We are only interested in the property kAudioQueueProperty_IsRunning
    if (inID != kAudioQueueProperty_IsRunning) return;

	struct myAQStruct *myInfo = (struct myAQStruct *)inUserData;

	/* Get the current status of the AQ, running or stopped */ 
    UInt32 isQueueRunning = false;
    UInt32 size = sizeof(isQueueRunning);
    AudioQueueGetProperty(myInfo->mQueue, kAudioQueueProperty_IsRunning, &isQueueRunning, &size);

	/* The callback event is the start of the queue */
    if (isQueueRunning) {
		/* reset current packet counter */
        myInfo->mCurrentPacket = 0;

        for (int i = 0; i < 3; i++) {
			/*
			 * For the first time allocate buffers for this AQ.
			 * Buffers are reused in turns until the AQ stops 
			 */
            AudioQueueAllocateBuffer(myInfo->mQueue, bufferSizeInSamples * 4, &myInfo->mBuffers[i]);

            UInt32 bytesRead = bufferSizeInSamples * 4;
            UInt32 packetsRead = bufferSizeInSamples;

			/*
			 * Read data from audio source into the buffer of AQ
			 * supplied in this callback event. Buffers are used in turns
			 * to hide the latency
			 */
            AudioFileReadPacketData(
					myInfo->mAudioFile,
					false, /* isUseCache, set to false */
					&bytesRead,
					NULL,
					myInfo->mCurrentPacket,
					&packetsRead,
					myInfo->mBuffers[i]->mAudioData);
			/* in case the buffer size is smaller than bytes requestd to read */ 
            myInfo->mBuffers[i]->mAudioDataByteSize = bytesRead;
            myInfo->mCurrentPacket += packetsRead;

            AudioQueueEnqueueBuffer(myInfo->mQueue, myInfo->mBuffers[i], 0, NULL);
        }
    } else {
		/* The callback event is the state of AQ changed to not running */
        if (myInfo->mAudioFile != NULL) {
			AudioQueueStop(myInfo->mQueue, false);
            AudioFileClose(myInfo->mAudioFile);
            myInfo->mAudioFile = NULL;

            for (int i = 0; i < 3; i++) {
                AudioQueueFreeBuffer(myInfo->mQueue, myInfo->mBuffers[i]);
                myInfo->mBuffers[i] = NULL;
            }
			CFRunLoopStop(CFRunLoopGetCurrent());
        }
    }
}

int
main (int argc, const char *argv[])
{
    CFURLRef audioFileURLRef;
    OSStatus ret;

    audioFileURLRef = CFURLCreateWithFileSystemPath (
            kCFAllocatorDefault,
			/* CFSTR ("../misc/test.wav"), */
			CFSTR ("../misc/alin.wav"),
            kCFURLPOSIXPathStyle,
            FALSE
            );

    ret = AudioFileOpenURL(
            audioFileURLRef,
            kAudioFileReadWritePermission,
            0,
            &myInfo.mAudioFile);
    if (ret != noErr) {
        printf("fail to open audio file %x\n", ret);
        return 1;
    }

    UInt32 propSize = sizeof(myInfo.mDataFormat);
    ret = AudioFileGetProperty(
            myInfo.mAudioFile,
            kAudioFilePropertyDataFormat,
            &propSize,
            &myInfo.mDataFormat
            );
    if (ret != noErr) {
        printf("AudioFileGetProperty error code %d\n", ret);
        return 1;
    }
    printf("sample rate: %f\n"
            "mFormatID: %u\n"
            "mFormatFlags: %u\n"
            "mBytesPerPacket: %u\n"
            "mChannelsPerFrame: %u\n",
            myInfo.mDataFormat.mSampleRate,
            myInfo.mDataFormat.mFormatID,
            myInfo.mDataFormat.mFormatFlags,
            myInfo.mDataFormat.mBytesPerPacket,
            myInfo.mDataFormat.mChannelsPerFrame
          );

    // Instantiate an audio queue object
    ret = AudioQueueNewOutput(
            &myInfo.mDataFormat,
            AQTestBufferCallback,
            &myInfo,
            CFRunLoopGetCurrent(),
            kCFRunLoopCommonModes,
            0,
            &myInfo.mQueue
            );
    if (ret != noErr) {
        printf("AudioQueueNewOutput error code %d\n", ret);
        return 1;
    }

    AudioQueueAddPropertyListener(myInfo.mQueue, kAudioQueueProperty_IsRunning,
            AudioEnginePropertyListenerProc, &myInfo);
    

    /* FIXME allocate AudioQueue buffer */ 
    int i;
    for (i = 0; i < 3; i++) {
        AudioQueueAllocateBuffer(myInfo.mQueue, 441 * 4, &myInfo.mBuffers[i]);
    }
    AudioQueueStart(myInfo.mQueue, NULL);

    printf("Run loop\n");
    // create a system sound ID to represent the sound file
    /* OSStatus error = AudioServicesCreateSystemSoundID (myURLRef, &mySSID); */

    // Register the sound completion callback.
    // Again, useful when you need to free memory after playing.
    /* AudioServicesAddSystemSoundCompletion ( */
            /* mySSID, */
            /* NULL, */
            /* NULL, */
            /* MyCompletionCallback, */
            /* (void *) myURLRef */
            /* ); */

    // Play the sound file.
    /* AudioServicesPlaySystemSound (mySSID); */

    // Invoke a run loop on the current thread to keep the application
    // running long enough for the sound to play; the sound completion
    // callback later stops this run loop.
    CFRunLoopRun ();
    return 0;
}

