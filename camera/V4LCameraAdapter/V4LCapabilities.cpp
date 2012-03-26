/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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

/**
* @file V4LCapabilities.cpp
*
* This file implements the V4L Capabilities feature.
*
*/

#include "CameraHal.h"
#include "V4LCameraAdapter.h"
#include "ErrorUtils.h"
#include "TICameraParameters.h"

namespace android {

/************************************
 * global constants and variables
 *************************************/

#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))
#define MAX_RES_STRING_LENGTH 10
#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

static const char PARAM_SEP[] = ",";

//Camera defaults
const char V4LCameraAdapter::DEFAULT_PICTURE_FORMAT[] = "jpeg";
const char V4LCameraAdapter::DEFAULT_PICTURE_SIZE[] = "640x480";
const char V4LCameraAdapter::DEFAULT_PREVIEW_FORMAT[] = "yuv422i";
const char V4LCameraAdapter::DEFAULT_PREVIEW_SIZE[] = "640x480";
const char V4LCameraAdapter::DEFAULT_NUM_PREV_BUFS[] = "6";
const char V4LCameraAdapter::DEFAULT_FRAMERATE[] = "30";


const CapPixelformat V4LCameraAdapter::mPixelformats [] = {
    { V4L2_PIX_FMT_YUYV, CameraParameters::PIXEL_FORMAT_YUV422I },
    { V4L2_PIX_FMT_JPEG, CameraParameters::PIXEL_FORMAT_JPEG },
};

/*****************************************
 * internal static function declarations
 *****************************************/

/**** Utility functions to help translate V4L Caps to Parameter ****/

status_t V4LCameraAdapter::insertDefaults(CameraProperties::Properties* params, V4L_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    params->set(CameraProperties::PREVIEW_FORMAT, DEFAULT_PREVIEW_FORMAT);

    params->set(CameraProperties::PICTURE_FORMAT, DEFAULT_PICTURE_FORMAT);
    params->set(CameraProperties::PICTURE_SIZE, DEFAULT_PICTURE_SIZE);
    params->set(CameraProperties::PREVIEW_SIZE, DEFAULT_PREVIEW_SIZE);
    params->set(CameraProperties::PREVIEW_FRAME_RATE, DEFAULT_FRAMERATE);
    params->set(CameraProperties::REQUIRED_PREVIEW_BUFS, DEFAULT_NUM_PREV_BUFS);

    params->set(CameraProperties::CAMERA_NAME, "USBCAMERA");
    params->set(CameraProperties::JPEG_THUMBNAIL_SIZE, "320x240");
    params->set(CameraProperties::FRAMERATE_RANGE_SUPPORTED, "30000,30000");
    params->set(CameraProperties::FRAMERATE_RANGE, "30000,30000");
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t V4LCameraAdapter::insertPreviewFormats(CameraProperties::Properties* params, V4L_TI_CAPTYPE &caps) {

    char supported[MAX_PROP_VALUE_LENGTH];

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);
    for (int i = 0; i < caps.ulPreviewFormatCount; i++) {
        for (unsigned int j = 0; j < ARRAY_SIZE(mPixelformats); j++) {
            if(caps.ePreviewFormats[i] == mPixelformats[j].pixelformat ) {
                strncat (supported, mPixelformats[j].param, MAX_PROP_VALUE_LENGTH-1 );
                strncat (supported, PARAM_SEP, 1 );
            }
        }
    }
    strncat(supported, CameraParameters::PIXEL_FORMAT_YUV420P, MAX_PROP_VALUE_LENGTH - 1);
    params->set(CameraProperties::SUPPORTED_PREVIEW_FORMATS, supported);
    return NO_ERROR;
}

status_t V4LCameraAdapter::insertPreviewSizes(CameraProperties::Properties* params, V4L_TI_CAPTYPE &caps) {

    char supported[MAX_PROP_VALUE_LENGTH];

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);
    for (int i = 0; i < caps.ulPreviewResCount; i++) {
        strncat (supported, caps.tPreviewRes[i].param, MAX_PROP_VALUE_LENGTH-1 );
        strncat (supported, PARAM_SEP, 1 );
    }
    params->set(CameraProperties::SUPPORTED_PREVIEW_SIZES, supported);
    return NO_ERROR;
}

status_t V4LCameraAdapter::insertImageSizes(CameraProperties::Properties* params, V4L_TI_CAPTYPE &caps) {

    char supported[MAX_PROP_VALUE_LENGTH];

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);
    for (int i = 0; i < caps.ulCaptureResCount; i++) {
        strncat (supported, caps.tCaptureRes[i].param, MAX_PROP_VALUE_LENGTH-1 );
        strncat (supported, PARAM_SEP, 1 );
    }
    params->set(CameraProperties::SUPPORTED_PICTURE_SIZES, supported);
    return NO_ERROR;
}

status_t V4LCameraAdapter::insertFrameRates(CameraProperties::Properties* params, V4L_TI_CAPTYPE &caps) {

    char supported[MAX_PROP_VALUE_LENGTH];

    memset(supported, '\0', MAX_PROP_VALUE_LENGTH);
    for (int i = 0; i < caps.ulFrameRateCount; i++) {
        snprintf (supported, 10, "%d", caps.ulFrameRates[i]*1000 );
        strncat (supported, PARAM_SEP, 1 );
    }
    params->set(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES, supported);
    return NO_ERROR;
}

status_t V4LCameraAdapter::insertCapabilities(CameraProperties::Properties* params, V4L_TI_CAPTYPE &caps)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NO_ERROR == ret ) {
        ret = insertPreviewFormats(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertImageSizes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertPreviewSizes(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertFrameRates(params, caps);
    }

    if ( NO_ERROR == ret ) {
        ret = insertDefaults(params, caps);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/*****************************************
 * public exposed function declarations
 *****************************************/

status_t V4LCameraAdapter::getCaps(const int sensorId, CameraProperties::Properties* params,
                                   V4L_HANDLETYPE handle) {
     status_t status = NO_ERROR;
     V4L_TI_CAPTYPE caps;
     int i = 0;
     struct v4l2_fmtdesc fmtDesc;
     struct v4l2_frmsizeenum frmSizeEnum;
     struct v4l2_frmivalenum frmIvalEnum;

    //get supported pixel formats
    for ( i = 0; status == NO_ERROR; i++) {
        fmtDesc.index = i;
        fmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        status = ioctl (handle, VIDIOC_ENUM_FMT, &fmtDesc);
        if (status == NO_ERROR) {
            CAMHAL_LOGDB("fmtDesc[%d].description::pixelformat::flags== (%s::%d::%d)",i, fmtDesc.description,fmtDesc.pixelformat,fmtDesc.flags);
            caps.ePreviewFormats[i] = fmtDesc.pixelformat;
        }
    }
    caps.ulPreviewFormatCount = i;

    //get preview sizes & capture image sizes
    status = NO_ERROR;
    for ( i = 0; status == NO_ERROR; i++) {
        frmSizeEnum.index = i;
        //Check for frame sizes for default pixel format
        //TODO: Check for frame sizes for all supported pixel formats
        frmSizeEnum.pixel_format = V4L2_PIX_FMT_YUYV;
        status = ioctl (handle, VIDIOC_ENUM_FRAMESIZES, &frmSizeEnum);
        if(frmSizeEnum.type != V4L2_FRMSIZE_TYPE_DISCRETE) {
            break;
        }
        if (status == NO_ERROR) {
            CAMHAL_LOGDB("frmSizeEnum.index[%d].width x height == (%d x %d)", i, frmSizeEnum.discrete.width, frmSizeEnum.discrete.height);
            caps.tPreviewRes[i].width = frmSizeEnum.discrete.width;
            caps.tPreviewRes[i].height = frmSizeEnum.discrete.height;
            snprintf(caps.tPreviewRes[i].param, MAX_RES_STRING_LENGTH,"%dx%d",frmSizeEnum.discrete.width,frmSizeEnum.discrete.height);

            caps.tCaptureRes[i].width = frmSizeEnum.discrete.width;
            caps.tCaptureRes[i].height = frmSizeEnum.discrete.height;
            snprintf(caps.tCaptureRes[i].param, MAX_RES_STRING_LENGTH,"%dx%d",frmSizeEnum.discrete.width,frmSizeEnum.discrete.height);
        }
        else {
            caps.ulCaptureResCount = i;
            caps.ulPreviewResCount = i;
        }
    }
    if(frmSizeEnum.type != V4L2_FRMSIZE_TYPE_DISCRETE) {
        CAMHAL_LOGDB("\nmin_width x height = %d x %d ",frmSizeEnum.stepwise.min_width, frmSizeEnum.stepwise.min_height);
        CAMHAL_LOGDB("\nmax_width x height = %d x %d ",frmSizeEnum.stepwise.max_width, frmSizeEnum.stepwise.max_height);
        CAMHAL_LOGDB("\nstep width x height = %d x %d ",frmSizeEnum.stepwise.step_width,frmSizeEnum.stepwise.step_height);
        //TODO: populate the sizes when type = V4L2_FRMSIZE_TYPE_STEPWISE
    }

    //get supported frame rates
    status = NO_ERROR;
    for ( i = 0; status == NO_ERROR; i++) {
        frmIvalEnum.index = i;
        //Check for supported frame rates for the default pixel format and default image frame size.
        //TODO: Check supported frame rates for all supported pixel format + resolution 
        frmIvalEnum.pixel_format = V4L2_PIX_FMT_YUYV;
        frmIvalEnum.width = DEFAULT_WIDTH;
        frmIvalEnum.height = DEFAULT_HEIGHT;
        status = ioctl (handle, VIDIOC_ENUM_FRAMEINTERVALS, &frmIvalEnum);
        if(frmIvalEnum.type != V4L2_FRMIVAL_TYPE_DISCRETE) {
            break;
        }
        if (status == NO_ERROR) {
            LOGD("frmIvalEnum[%d].frame rate= %d)",i, (frmIvalEnum.discrete.denominator/frmIvalEnum.discrete.numerator));
            caps.ulFrameRates[i] = (frmIvalEnum.discrete.denominator/frmIvalEnum.discrete.numerator);
        }
        else {
            caps.ulFrameRateCount = i;
        }
    }
    if(frmIvalEnum.type != V4L2_FRMIVAL_TYPE_DISCRETE) {
        //TODO: populate the frame rates when type = V4L2_FRMIVAL_TYPE_STEPWISE;
    }

    insertCapabilities (params, caps);
    return NO_ERROR;
}



};