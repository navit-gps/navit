#import  <foundation/foundation.h>

@interface VSSpeechSynthesizer : NSObject
{
}

+ (id)availableLanguageCodes;
+ (BOOL)isSystemSpeaking;
- (id)startSpeakingString:(id)string;
- (id)startSpeakingString:(id)string toURL:(id)url;
- (id)startSpeakingString:(id)string toURL:(id)url withLanguageCode:(id)code;
- (float)rate;             // default rate: 1
- (id)setRate:(float)rate;
- (float)pitch;           // default pitch: 0.5
- (id)setPitch:(float)pitch;
- (float)volume;       // default volume: 0.8
- (id)setVolume:(float)volume;
@end
