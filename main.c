/* 
   
   PSP Recording Example v0.2

   Application intended to demonstrate the use of the audio input API
   by looping back the audio input to the output.

   by lteixeira (teixeluis@gmail.com)
   
*/

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspaudiolib.h>
#include <pspaudio.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <psphprm.h>

#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "psp_audio_ext.h"
#include "logging.h"


PSP_MODULE_INFO("PSP_Recording_Example", 0, 1, 1);
/* Define the main thread's attribute value (optional) */
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

/* Define printf, just to make typing easier */
#define printf	pspDebugScreenPrintf

sample_t* loop_buffer;

short mic_level;

/* Exit callback */
int exitCallback(int arg1, int arg2, void *common) {
	sceKernelExitGame();
	return 0;
}

/* Callback thread */
int callbackThread(SceSize args, void *argp) {
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", (void*) exitCallback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();

	return 0;
}

/* Sets up the callback thread and returns its thread id */
int setupCallbacks(void) {
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", callbackThread, 0x11, 0xFA0, 0, 0);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, 0);
	}
	return thid;
}

/* This function gets called by pspaudiolib every time the
   audio output buffer needs to be filled. The sample format is
   16-bit, stereo. */
   
void audioOutputLoopCallback(void* buf, unsigned int length, void *userdata) {
    sample_t* ubuf = (sample_t*) buf;
	int i;

	// Fill the output buffer with the recorded contents
	// stored in main_buffer:
	
	for (i = 0; i < length; i++) {
		ubuf[i].l = loop_buffer[i].l;
		ubuf[i].r = loop_buffer[i].r;
	}
}

/* This function gets called by pspaudiolib every time the
   audio input buffer needs to be drained. The sample format is
   16-bit, stereo. */
   
void audioInputLoopCallback(void* buf, unsigned int length, void *userdata) {
	short* ubuf = (short*) buf;
	int i;

	// Fill the main buffer with the recorded contents
	// stored in the input buffer:
	
	for (i = 0; i < length; i++) {
		loop_buffer[i].l = ubuf[i];
		loop_buffer[i].r = ubuf[i];
	}
}

void audioLoopStart() {
	printLog("audioLoopStart: function called.\n");
	printLog("audioLoopStart: loop_buffer addr = %8X\n", (int) loop_buffer);
	
	loop_buffer = (sample_t*) malloc(PSP_NUM_AUDIO_SAMPLES * sizeof(sample_t));
	printLog("audioLoopStart: loop_buffer space allocated.\n");

	pspAudioInputInit(mic_level, RECORD_SAMPLE_RATE);
	printLog("audioLoopStart: pspAudioInputInit successfully invoked.\n");

	sceKernelDelayThread(200000);	
	pspAudioSetInputCallback(audioInputLoopCallback, NULL);
	printLog("audioLoopStart: pspAudioSetInputCallback successfully invoked.\n");

	pspAudioSetChannelCallback(0, audioOutputLoopCallback, NULL);
	printLog("audioLoopStart: pspAudioSetChannelCallback successfully invoked.\n");
}

void testAudioInputInit() {
	int input_length;
	
	//sceAudioInputInit(0, mic_level, 0);
	pspAudioInputInit(mic_level, RECORD_SAMPLE_RATE);
	printLog("testAudioInputInit: pspAudioInputInit successfully invoked.\n");
	
	input_length = sceAudioGetInputLength();
	
	printLog("testAudioInputInit: result of sceAudioGetInputLength = %d\n", input_length); 
}

void audioLoopStop() {
	printLog("audioLoopStop: function called.\n");
 	//pspAudioSetInputCallback(NULL, NULL);
	pspAudioSetChannelCallback(0, NULL, NULL);
	pspAudioInputEnd();	
	if(loop_buffer != NULL) {
		free(loop_buffer);
		loop_buffer = NULL;
	}
	printLog("audioLoopStop: pspAudioSetChannelCallback successfully invoked.\n");
}

int main(int argc, char* argv[]) {
	SceCtrlData pad;
	int changedButtons = 0;
	int oldButtons = 0;
	int command = -1;
	int sound_loop_state = 0;
	char sound_loop_state_str[3];

	mic_level = 4096;
	loop_buffer = NULL;

	pspDebugScreenInit();
	setupCallbacks();

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	
	printMenu();
	pspAudioInit();
		
	for(;;) {
		sceDisplayWaitVblankStart();

		sceCtrlReadBufferPositive(&pad, 1);
		changedButtons = pad.Buttons & (~oldButtons);		
	
		if(changedButtons & PSP_CTRL_CROSS) {
	        command = TOGGLE_SOUND_LOOP;
		}
		else if(changedButtons & PSP_CTRL_CIRCLE) {
			command = EXIT_PROG;
		}
		else if(changedButtons & PSP_CTRL_SQUARE) {
			command = TEST_AUDIO_INPUT_INIT;
		}			
		else if(changedButtons == 0)
			command = NO_COMMAND;		
		else
			command = UNKNOWN_COMMAND;
			
		oldButtons = pad.Buttons;
		
		switch(command) {
			case TOGGLE_SOUND_LOOP:
				sound_loop_state = !sound_loop_state;
				strcpy(sound_loop_state_str, (sound_loop_state ? "on":"off"));
			 	printf("Turning sound loop %s.\n", sound_loop_state_str);
				
				if(sound_loop_state) {
					printLog("main: called audioLoopStart()\n");					
					audioLoopStart();
					printLog("main: returned from audioLoopStart()\n");
				}
				else {
					printLog("main: called audioLoopStop()\n");
					audioLoopStop();
					printLog("main: returned from audioLoopStop()\n");
				}
			  	break;
			case TEST_AUDIO_INPUT_INIT:
				testAudioInputInit();
				break;
			case EXIT_PROG:
				printf("Exiting program..\n");
				pspAudioEnd();
				sceKernelExitGame();
			 	return 0;
			case NO_COMMAND:
				break;
		 	default:
		 		printMenu();
		 		break;
		}
		command = NO_COMMAND;		
	}
		
}

void printMenu() {
	pspDebugScreenClear();
	printf("PSP Recording Example v0.2\n\n");
	printf("Options\n");
	printf("=======\n\n");
	printf("* Press X to start or stop recording in loop mode.\n");
	printf("* Press O to exit.\n\n");
}
