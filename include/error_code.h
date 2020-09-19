#pragma once
namespace video {
 
enum class ErrorCode {
    kOk, 
    kChannelNotFound, 
    kTimeout, 
    kChannelHasBeenRun, 
    kChannelIsNotRun, 
	kChannelStartFail,
    kReaderCreatorNotFound, 
    kDecoderCreatorNotFound, 

    kReaderHasBeenCreated, 
    kCreateReaderFail, 
    kReaderIsNotCreated, 
    kReadFail, 

    kDecoderHasBeenCreated, 
    kCreateDecoderFail, 
    kDecoderIsNotCreated, 
    kDecodeFail, 
};

}
