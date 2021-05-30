//
//  VSSpeechSynthesizer.m
//  speech_iphone
//
//  Created by olf on 27.05.21.
//

#import <Foundation/Foundation.h>
#include "VSSpeechSynthesizer.h"
#import <AVFoundation/AVFoundation.h>

AVSpeechUtterance *utterance;
AVSpeechSynthesizer *synth;

@implementation VSSpeechSynthesizer

- (id)init {
    self = [super init];

    if (self) {
        //utterance = [AVSpeechUtterance speechUtteranceWithString:@"Welcome to navit"];
        synth = [[AVSpeechSynthesizer alloc] init];
        utterance.volume=90.0f;
        utterance.rate=0.50f;
        utterance.pitchMultiplier=0.80f;
        NSString * language = [[NSLocale preferredLanguages] firstObject];
        utterance.voice = [AVSpeechSynthesisVoice voiceWithLanguage:language];
        //[synth speakUtterance:utterance];

    }

    return self;
}

+ (id)availableLanguageCodes {

    return NULL;
}

+ (BOOL)isSystemSpeaking {
    return false;
}

- (id)startSpeakingString:(id)string {
    utterance = [AVSpeechUtterance speechUtteranceWithString:string];
    utterance.volume=90.0f;
    utterance.rate=0.50f;
    utterance.pitchMultiplier=0.80f;
    NSString * language = [[NSLocale preferredLanguages] firstObject];
    utterance.voice = [AVSpeechSynthesisVoice voiceWithLanguage:language];
    [synth speakUtterance:utterance];
}

- (id)startSpeakingString:(id)string toURL:(id)url {

}

- (id)startSpeakingString:(id)string toURL:(id)url withLanguageCode:(id)code {

}

- (float)rate {
    return utterance.rate;
}

- (id)setRate:(float)rate {

}

- (float)pitch {
    return utterance.pitchMultiplier;
}

- (id)setPitch:(float)pitch {
    utterance.pitchMultiplier = pitch;
}

- (float)volume {
    return utterance.volume;
}

- (id)setVolume:(float)volume {
    utterance.volume=volume;
}

@end
