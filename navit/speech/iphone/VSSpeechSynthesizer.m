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
AVAudioSession *session;

float volume;
float rate;
float pitch;
int use_hfp;
int force_hfp;
int use_mix_to_telephony_uplink;
int current_is_hfp;
int interrupted;
double hfpdelay;

@implementation VSSpeechSynthesizer

- (id)init {

    self = [super init];

    if (self) {
        synth = [[AVSpeechSynthesizer alloc] init];
        session = [AVAudioSession sharedInstance];
        use_hfp = YES;
        force_hfp = NO;
        current_is_hfp = NO;

        synth.delegate = self;

        NSNotificationCenter *notficationcenter = NSNotificationCenter.defaultCenter;
        [notficationcenter addObserver:self selector:@selector(handleInterruption:) name:
                           AVAudioSessionInterruptionNotification object: session];

    }
    return self;
}

- (BOOL)isInterrupted {
    return interrupted;
}

- (void) handleInterruption:(NSNotification*)note {
    AVAudioSessionInterruptionType interruptionType = [[[note userInfo]
            objectForKey:AVAudioSessionInterruptionTypeKey] unsignedIntegerValue];
    if (AVAudioSessionInterruptionTypeBegan == interruptionType) {
        NSLog (@"AVAudioSession interrupted");
        interrupted=YES;
    } else if (AVAudioSessionInterruptionTypeEnded == interruptionType) {
        NSLog (@"AVAudioSession interruption ended");
        interrupted=NO;
    }
}

+ (BOOL)isSystemSpeaking {
    return synth.speaking;
}

- (void)speechSynthesizer:(AVSpeechSynthesizer *)synthesizer
    didFinishSpeechUtterance:(AVSpeechUtterance *)utterance {

    NSError *activationErr = nil;

    // Deactivate session but not for the silent HFP announcement
    if(strcmp(utterance.speechString.UTF8String, "XantippeXantippe")) {
        [session setActive:false error:&activationErr];
    }
}

- (id)startSpeakingString:(id)string {

    NSError *activationErr = nil;

    // Don't try to play announcements if the session is interrupted, else they all get queued and will be played
    // when the interruption ends
    if(!self.isInterrupted) {

        [self useHFP:1 force:2];    // Checks before each announcement if there is background audio playing
        // If that is detected, we use A2DP and not HFP. force:2 will keep current
        // settings for force_hfp and useHFP.
        [session setActive:true error:&activationErr];

        utterance = [AVSpeechUtterance speechUtteranceWithString:string];

        // We use a configurable delay (speech_hfp_delay in speech tag in navit.xml)
        // to give time to establish a HFP connection so the navigation
        // announcement will not get trunkated at the beginning
        if(current_is_hfp) {
            NSLog (@"AVSpeechUtterance delayed HFP by: %1.2f Error: %@", hfpdelay, activationErr.localizedFailureReason);
            [utterance setPreUtteranceDelay:(hfpdelay)];
        }

        NSLog (@"AVSpeechUtterance play navigation announcement: %@ Error: %@", utterance.speechString,
               activationErr.localizedFailureReason);

        utterance.rate=rate;
        utterance.pitchMultiplier=pitch;
        utterance.volume=volume;
        [synth speakUtterance:utterance];
        NSLog (@"AVSpeechUtterance play navigation announcement: %@ Error: %@", utterance.speechString,
               activationErr.localizedFailureReason);
    }

    return 0;
}

- (void) setHFPDelay:(double) delay {
    hfpdelay = delay;
    NSLog (@"setHFPDelay: %1.2f", delay);
}

- (id)startSpeakingString:(id)string toURL:(id)url {
    return 0;
}

- (id)startSpeakingString:(id)string toURL:(id)url withLanguageCode:(id)code {
    return 0;
}

- (float)rate {
    return rate;
}

- (id)setRate:(float)newrate {
    rate = newrate;
    return 0;
}

- (float)pitch {
    return pitch;
}

- (id)setPitch:(float)newpitch {
    pitch = newpitch;
    return 0;
}

- (float)volume {
    return volume;
}

- (id)setVolume:(float)newvolume {
    volume = newvolume;
    return 0;
}

- (id)useHFP:(int)newuse_hfp force:(int)force {

    NSError *setCategoryErr = nil;

    // Change only for NO and YES
    if(force < 2) {
        force_hfp = force;
        use_hfp = newuse_hfp;
    }

    // Get current options
    AVAudioSessionCategoryOptions options = [session categoryOptions];

    if((use_hfp && force_hfp) || (((use_hfp || newuse_hfp) && !force_hfp) && ![session isOtherAudioPlaying])) {
        // Force usage of HFP (only if no background music is playing) or if you explicily requested to use it
        // always by setting 'speech_use_hfp="1"' in speech tag in navit.xml:
        // radio gets muted while playing announcements,
        // but music playback in background would switch to HFP as well during announcement playback.
        // TODO: Do we want the speaker as default?

        current_is_hfp = YES;

        if(!(options & AVAudioSessionCategoryOptionAllowBluetooth))
            [session setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:AVAudioSessionCategoryOptionAllowBluetooth |
                     AVAudioSessionCategoryOptionDuckOthers   error:&setCategoryErr];
        NSLog(@"AVAudioSession HFP: %@", setCategoryErr.localizedFailureReason);

    } else {

        current_is_hfp = NO;

        // Allow A2DP: No radio mute, but better voice quality
        if(!(options & AVAudioSessionCategoryOptionAllowBluetoothA2DP))
            [session setCategory:AVAudioSessionCategoryPlayback withOptions:AVAudioSessionCategoryOptionAllowBluetoothA2DP |
                     AVAudioSessionCategoryOptionDuckOthers   error:&setCategoryErr];
        NSLog(@"AVAudioSession A2DP: %@", setCategoryErr.localizedFailureReason);
    }

    return 0;
}

- (BOOL)is_useHFP {
    return use_hfp;
}

@end
