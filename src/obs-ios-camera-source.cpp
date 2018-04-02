/*
obs-ios-camera-source
Copyright (C) 2018	Will Townsend <will@townsend.io>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <chrono>
#include <Portal.hpp>
#include <usbmuxd.h>
#include <obs-avc.h>

#include "FFMpegVideoDecoder.h"
#include "VideoToolboxVideoDecoder.h"

#define TEXT_INPUT_NAME obs_module_text("OBSIOSCamera.Title")


class IOSCameraInput : public portal::PortalDelegate, public VideoToolboxDecoderCallback
{
  public:
	obs_source_t *source;
	portal::Portal portal;
//    Decoder video_decoder;
	bool active = false;
	obs_source_frame frame;
    
    VideoToolboxDecoder decoder;
    FFMpegVideoDecoder ffDecoder;

	inline IOSCameraInput(obs_source_t *source_, obs_data_t *settings)
		: source(source_)
	{
		memset(&frame, 0, sizeof(frame));

		portal.delegate = this;
		active = true;
        
        decoder.mDelegate = this;
        
        ffDecoder.source = source;

        obs_source_set_async_unbuffered(source, true);
        
		blog(LOG_INFO, "Started listening for devices");
	}

	inline ~IOSCameraInput()
	{
		portal.stopListeningForDevices();
		
	}

    void activate() {
        blog(LOG_INFO, "Activating");
        portal.startListeningForDevices();
        portal.connectAllDevices();
    }
    
    void deactivate() {
        blog(LOG_INFO, "Deactivating");
        portal.disconnectAllDevices();
    }
    
	void portalDeviceDidReceivePacket(std::vector<char> packet)
	{
//        unsigned char *data = (unsigned char *)packet.data();
//        long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//        OnEncodedVideoData(AV_CODEC_ID_H264, data, packet.size(), now);
        
        this->decoder.Input(packet);
//        this->ffDecoder.Input(packet);
        
	}

//    void OnEncodedVideoData(enum AVCodecID id,
//                            unsigned char *data, size_t size, long long ts)
//    {
//        if (!ffmpeg_decode_valid(video_decoder))
//        {
//            if (ffmpeg_decode_init(video_decoder, id) < 0)
//            {
//                blog(LOG_WARNING, "Could not initialize video decoder");
//                return;
//            }
//        }
//
//        bool got_output;
//        bool success = ffmpeg_decode_video(video_decoder, data, size, &ts,
//                                           &frame, &got_output);
//        if (!success)
//        {
//            blog(LOG_WARNING, "Error decoding video");
//            return;
//        }
//
//        if (got_output)
//        {
//            frame.timestamp = (uint64_t)ts * 100;
//            //if (flip)
//            //frame.flip = !frame.flip;
//#if LOG_ENCODED_VIDEO_TS
//            blog(LOG_DEBUG, "video ts: %llu", frame.timestamp);
//#endif
//            obs_source_output_video(source, &frame);
//        }
//    }
    
    void VideoToolboxDecodedFrame(CVPixelBufferRef pixelBufferRef, CMVideoFormatDescriptionRef formatDescription)
    {
        
        CVImageBufferRef     image = pixelBufferRef;
//        obs_source_frame *frame = frame;
        
        // CMTime target_pts =
        // CMSampleBufferGetOutputPresentationTimeStamp(sampleBuffer);
        // CMTime target_pts_nano = CMTimeConvertScale(target_pts, NANO_TIMESCALE,
        // kCMTimeRoundingMethod_Default);
        // frame->timestamp = target_pts_nano.value;
        
        
        
        if (!this->decoder.update_frame(source, &frame, image, formatDescription)) {
            // Send blank video
             obs_source_output_video(source, nullptr);
             return;
        }
        
        obs_source_output_video(source, &frame);
        
        CVPixelBufferUnlockBaseAddress(image, kCVPixelBufferLock_ReadOnly);
    }
};

static const char *GetIOSCameraInputName(void *)
{
	return TEXT_INPUT_NAME;
}

static void *CreateIOSCameraInput(obs_data_t *settings, obs_source_t *source)
{
	IOSCameraInput *cameraInput = nullptr;

	try
	{
		cameraInput = new IOSCameraInput(source, settings);
	}
	catch (const char *error)
	{
		blog(LOG_ERROR, "Could not create device '%s': %s", obs_source_get_name(source), error);
	}

	return cameraInput;
}

static void DestroyIOSCameraInput(void *data)
{
	delete reinterpret_cast<IOSCameraInput *>(data);
}

static void DeactivateIOSCameraInput(void *data)
{
    auto cameraInput =  reinterpret_cast<IOSCameraInput*>(data);
    cameraInput->deactivate();
}

static void ActivateIOSCameraInput(void *data)
{
    auto cameraInput = reinterpret_cast<IOSCameraInput*>(data);
    cameraInput->activate();
}

void RegisterIOSCameraSource()
{
	obs_source_info info = {};
	info.id              = "ios-camera-source";
	info.type            = OBS_SOURCE_TYPE_INPUT;
	info.output_flags    = OBS_SOURCE_ASYNC_VIDEO;
	info.get_name        = GetIOSCameraInputName;
	info.create          = CreateIOSCameraInput;
	info.destroy         = DestroyIOSCameraInput;
    info.deactivate      = DeactivateIOSCameraInput;
    info.activate        = ActivateIOSCameraInput;
	// info.update          = UpdateDShowInput;
	// info.get_defaults    = GetDShowDefaults;
	// info.get_properties  = GetDShowProperties;
	obs_register_source(&info);
}