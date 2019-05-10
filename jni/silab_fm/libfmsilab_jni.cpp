/* SILAB 47xx
 *
 * Copyright (C) 2019 Draekko, Ben Touchette
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <jni.h>
#include "fmr.h"
#include <linux/types.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "silab_fm.h"
#include "silab_ioctl.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "LIBFMSILAB_JNI"


char TEXT_GROUP_2A[65];
char LRT_GROUP_2A[65];
bool wait2a = true;

struct radio_data_t rds;
int scanning = 0;

jboolean openDev(JNIEnv *env, jobject thiz) {
    int ret = 0;
    int fd = open("/dev/radio0", O_RDWR);
	if (fd < 0) {
        LOGE("Unable to open /dev/radio\n");
        return JNI_FALSE;
    }
    setFd(fd);

    for (int p = 0; p < 64; p++) {
        TEXT_GROUP_2A[p] = 32;
        LRT_GROUP_2A[p] = 32;
    }
    TEXT_GROUP_2A[64] = 0;
    LRT_GROUP_2A[64] = 0;

    return JNI_TRUE;
}

jboolean closeDev(JNIEnv *env, jobject thiz) {
    close(getFd());
    return JNI_TRUE;
}

jboolean powerUp(JNIEnv *env, jobject thiz, jfloat freq) {
    jboolean status = JNI_TRUE;

    LOGI("%s, [freq=%d]\n", __func__, (int)freq);

	if (powerup() != SILAB_TRUE) {
        status = JNI_FALSE;
    }

    if (setmuteon() != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	struct sys_config2 c2;
	c2.rssi_th = 0x1;
	c2.fm_band = BAND_87500_108000_kHz;
	c2.fm_chan_spac = CHAN_SPACING_100_kHz;
	if (setsysconfig2 (&c2) != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	struct sys_config3 c3;
	c3.sksnr = 0x2;
	if (setsysconfig3 (&c3) != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	u8 rssi_threshold = 0x1;
	if (setrssi_th (rssi_threshold) != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	u8 snr_threshold = 0x2;
	if (setsnr_th (snr_threshold) != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	u8 cnt_threshold = 0x0;
	if (setcnt_th (snr_threshold) != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	int band = BAND_87500_108000_kHz;
	if (setband(band) != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	int chansp = CHAN_SPACING_100_kHz;
	if (setchannelspacing(chansp) != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	u8 de_constant = DE_TIME_CONSTANT_50;
	if (setdeconstant(de_constant) != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (setstereo() != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (setvolume(8)  != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (setdsmuteon() != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (setmuteoff() != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (setmuteon()  != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (disablerds() != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	u32 frq = (u32)freq;
	if (setfreq(frq) != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (setmuteoff() != SILAB_TRUE) {
        status = JNI_FALSE;
    }

    return status;
}

jboolean powerDown(JNIEnv *env, jobject thiz, jint type) {
	if (powerdown() != SILAB_TRUE) {
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

jboolean tune(JNIEnv *env, jobject thiz, jfloat freq) {
    jboolean status = JNI_TRUE;

    LOGI("%s, [freq=%d]\n", __func__, (int)freq);

    wait2a = true;

	if (setdsmuteon() != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (setmuteon() != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (resetrds() != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	u32 frq = (u32)freq;
	if (setfreq(frq) != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (setmuteoff() != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (setvolume(0) != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (setmuteon() != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (setstereo() != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (setvolume(8) != SILAB_TRUE) {
        status = JNI_FALSE;
    }

	if (setmuteoff() != SILAB_TRUE) {
        status = JNI_FALSE;
    }

    return status;
}

jfloat seek(JNIEnv *env, jobject thiz, jfloat freq, jboolean isUp) {
    u32 frq;
    LOGI("%s, [freq=%d][SCAN %s]\n", __func__, (int)freq, isUp?"UP":"DOWN");
    wait2a = true;
    if (isUp == JNI_TRUE) {
	    if (seekup(&frq) != SILAB_TRUE) {
            return 0;
        }
    } else {
	    if (seekdown(&frq) != SILAB_TRUE) {
            return 0;
        }
    }
    LOGI("%s, [freq=%d]\n", __func__, frq);
    float retfreq = (float)((float)frq / 100.0f);
    LOGI("%s, [retfreq=%f]\n", __func__, retfreq);
    return retfreq;
}

jshortArray autoScan(JNIEnv *env, jobject thiz) {
    u32 bottomfreq = BOTTOM_FREQ_8750;
    u32 topfreq = TOP_FREQ_10800;

	if (setfreq(bottomfreq) != SILAB_TRUE) {
        return NULL;
    }

    jshort scanlist[MAX_FM_SCAN_CH_SIZE];

    scanning = 1;
    int count = 0;
    for (int chcount = 0; chcount < MAX_FM_SCAN_CH_SIZE; chcount++) {
        u32 seekfreq;
        if (seekup(&seekfreq) != SILAB_TRUE) {
            scanning = 0;
            return NULL;
        }
        LOGI("FOUND [%d KHz]\n", (int)seekfreq);
        scanlist[count] = seekfreq;
        if (scanning == 0) {
            return NULL;
        }
        if (seekfreq >= topfreq) {
            count+=1;
            scanning = 0;
            break;
        }
        count++;
    }
    scanning = 0;

    jshortArray scanlistings = env->NewShortArray(count);
    env->SetShortArrayRegion(scanlistings, 0, count, (const jshort*)&scanlist[0]);

    return scanlistings;
}

jshort readRds(JNIEnv *env, jobject thiz) {
    if (getrdsdata(&rds) == SILAB_TRUE)  {
#ifdef DEBUG_SILAB
        LOGI("RDS RSSI %d, FRQ %fMHz\n", rds.curr_rssi,  (float)rds.curr_channel / 100.0f);
        LOGI("RDS rdsabcd 0x%04X|%d 0x%04X|%d 0x%04X|%d 0x%04X|%d\n", rds.rdsa, rds.rdsa, rds.rdsb, rds.rdsb, rds.rdsc, rds.rdsc, rds.rdsd, rds.rdsd);
        LOGI("RDS blerabcd %d %d %d %d\n", rds.blera, rds.blerb, rds.blerc, rds.blerd);
#endif
        int picode = rds.rdsa;
        int grpcode = rds.rdsb >> 11;
        bool clear = false;
        if (grpcode == 0x1) {
            picode = rds.rdsc;
        } else if (grpcode == GROUP_TYPE_2A || grpcode == GROUP_TYPE_2B) {
            if ((rds.rdsb & 0x10) == 0x10) {
                clear = true;
            } else {
                clear = false;
            }
            int pos = rds.rdsb & 0xf;
            int pty = (rds.rdsb > 5) & 0x1f;
		    u8 c1 = (u8)(rds.rdsc >> 8);
		    u8 c2 = (u8)(rds.rdsc & 0xFF);
		    u8 c3 = (u8)(rds.rdsd >> 8);
		    u8 c4 = (u8)(rds.rdsd & 0xFF);
            if (c1 < 32 || c1 > 126) c1 = 32;
            if (c2 < 32 || c2 > 126) c2 = 32;
            if (c3 < 32 || c3 > 126) c3 = 32;
            if (c4 < 32 || c4 > 126) c4 = 32;
            if (pos == 0) {
                for (int p = 0; p < 65; p++) {
                    LRT_GROUP_2A[p] = TEXT_GROUP_2A[p];
                }
                for (int p = 0; p < 64; p++) {
                    TEXT_GROUP_2A[p] = 32;
                }
                TEXT_GROUP_2A[64] = 0;
            }

            TEXT_GROUP_2A[pos * 4 + 0] = (char)c1;
            TEXT_GROUP_2A[pos * 4 + 1] = (char)c2;
            TEXT_GROUP_2A[pos * 4 + 2] = (char)c3;
            TEXT_GROUP_2A[pos * 4 + 3] = (char)c4;

#ifdef DEBUG_SILAB
            LOGI("[%d] TXT[%c%c%c%c]", pos, c1, c2 , c3, c4);
            LOGI("RDS TEXT [%s]\n", TEXT_GROUP_2A);
#endif
            if (pos == 0) {
                if (wait2a == false) {
                    return RDS_EVENT_LAST_RADIOTEXT | RDS_EVENT_PTY;
                } else {
                    wait2a = false;
                }
            }

#if 0
        } else {
            //wait2a = true;
#endif
        }
    }

    return 0;
}

jbyteArray getPs(JNIEnv *env, jobject thiz) {
    return NULL;
}

jbyteArray getLrText(JNIEnv *env, jobject thiz) {
    LOGI("SEND LTR [%s]", LRT_GROUP_2A);
    jbyteArray radiotext64 = env->NewByteArray(65);
    env->SetByteArrayRegion(radiotext64, 0, 65, (const jbyte*)&LRT_GROUP_2A[0]);
    return radiotext64;
}

jshort activeAf(JNIEnv *env, jobject thiz) {
    u32 freq;
    if (getfreq (&freq) == SILAB_TRUE) {
        LOGI("Active AF %fMHz", (float)freq / 100.0f);
        return (jshort)freq;
    }
    return 0;
}

jint setRds(JNIEnv *env, jobject thiz, jboolean rdson) {
    if (rdson) {
        wait2a = true;
        disablerds();
        enablerds();
        resetrds();
        return JNI_TRUE;
    } else {
        disablerds();
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

jboolean stopScan(JNIEnv *env, jobject thiz) {
    if (scanning == 1) {
        scanning = 0;
    }
	if (seekstop() != SILAB_TRUE) {
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

jint setMute(JNIEnv *env, jobject thiz, jboolean mute) {
    if (mute) {
    	if (setmuteon() == SILAB_TRUE) {
            return JNI_TRUE;
        }
    } else {
    	if (setmuteoff() == SILAB_TRUE) {
            return JNI_TRUE;
        }
    }
    return JNI_FALSE;
}

static const char *classPathNameRx = "com/android/fmradio/FmNative";

static JNINativeMethod methodsRx[] = {
    {"openDev", "()Z", (void*)openDev },      //1
    {"closeDev", "()Z", (void*)closeDev },    //2
    {"powerUp", "(F)Z", (void*)powerUp },     //3
    {"powerDown", "(I)Z", (void*)powerDown }, //4
    {"tune", "(F)Z", (void*)tune },           //5
    {"seek", "(FZ)F", (void*)seek },          //6
    {"autoScan",  "()[S", (void*)autoScan },  //7
    {"stopScan",  "()Z", (void*)stopScan },   //8
    {"setRds",    "(Z)I", (void*)setRds  },   //10
    {"readRds",   "()S", (void*)readRds },    //11
    {"getPs",     "()[B", (void*)getPs  },    //12
    {"getLrText", "()[B", (void*)getLrText},  //13
    {"setMute",	"(Z)I", (void*)setMute},      //14
};

/*
 * Register several native methods for one class.
 */
static jint registerNativeMethods(JNIEnv* env, const char* className,
    JNINativeMethod* gMethods, int numMethods) {
    jclass clazz;

    clazz = env->FindClass(className);
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    if (clazz == NULL) {
        LOGE("Native registration unable to find class '%s'", className);
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        LOGE("RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }

    LOGD("%s, success\n", __func__);
    return JNI_TRUE;
}

/*
 * Register native methods for all classes we know about.
 *
 * returns JNI_TRUE on success.
 */
static jint registerNatives(JNIEnv* env) {
    jint ret = JNI_FALSE;

    if (registerNativeMethods(env, classPathNameRx,methodsRx,
        sizeof(methodsRx) / sizeof(methodsRx[0]))) {
        ret = JNI_TRUE;
    }

    LOGD("%s, done\n", __func__);
    return ret;
}

// ----------------------------------------------------------------------------

/*
 * This is called by the VM when the shared library is first loaded.
 */

typedef union {
    JNIEnv* env;
    void* venv;
} UnionJNIEnvToVoid;

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    UnionJNIEnvToVoid uenv;
    uenv.venv = NULL;
    jint result = -1;
    JNIEnv* env = NULL;

    LOGI("JNI_OnLoad");

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("ERROR: GetEnv failed");
        goto fail;
    }
    env = uenv.env;

    if (registerNatives(env) != JNI_TRUE) {
        LOGE("ERROR: registerNatives failed");
        goto fail;
    }

    result = JNI_VERSION_1_4;

fail:
    return result;
}

