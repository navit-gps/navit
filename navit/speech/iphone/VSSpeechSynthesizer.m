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

float volume;
float rate;
float pitch;

@implementation VSSpeechSynthesizer

- (id)init {
    self = [super init];

    if (self) {
        synth = [[AVSpeechSynthesizer alloc] init];
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
    utterance.volume=volume;
    utterance.rate=rate;
    utterance.pitchMultiplier=pitch;
    [synth speakUtterance:utterance];
}

- (id)startSpeakingString:(id)string toURL:(id)url {

}

- (id)startSpeakingString:(id)string toURL:(id)url withLanguageCode:(id)code {

}

- (float)rate {
    return rate;
}

- (id)setRate:(float)newrate {
    rate = newrate;
}

- (float)pitch {
    return pitch;
}

- (id)setPitch:(float)newpitch {
    pitch = newpitch;
}

- (float)volume {
    return volume;
}

- (id)setVolume:(float)newvolume {
    volume = newvolume;
}

@end
