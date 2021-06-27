#import  <foundation/foundation.h>
#import <AVFoundation/AVFoundation.h>

@interface VSSpeechSynthesizer : NSObject <AVSpeechSynthesizerDelegate>
{
}

//+ (id)availableLanguageCodes;
+ (BOOL)isSystemSpeaking;
- (id)startSpeakingString:(id)string;
- (id)startSpeakingString:(id)string toURL:(id)url;
- (id)startSpeakingString:(id)string toURL:(id)url withLanguageCode:(id)code;
- (float)rate;              // default rate: 1
- (id)setRate:(float)rate;
- (float)pitch;             // default pitch: 0.5
- (id)setPitch:(float)pitch;
- (float)volume;            // default volume: 0.8
- (id)setVolume:(float)volume;
- (id)useHFP:(int)use_hfp force:(int)force;  // We want to use HFP to announce, default true
- (BOOL)is_useHFP;
@end
