//
//  DownloaderDetailViewController.m
//  Downloader
//
//  Created by Nick Geoghegan on 09/08/2011.
//  Copyright 2011 Navit Project. All rights reserved.
//

#import "DownloaderDetailViewController.h"
#import "DownloaderConstants.h"

@implementation DownloaderDetailViewController

@synthesize locationName=locationName_;


// The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
/*
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization.
    }
    return self;
}
*/


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {

	// Sets the Navigation bar to have the location name
	self.title = [self.locationName objectForKey:NAME_KEY];

	[super viewDidLoad];
}


/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations.
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];

    // Release any cached data, images, etc. that aren't in use.
}

- (void)viewDidUnload {
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

-(void) viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
}

- (void)dealloc {
	[locationName_ release];
    [super dealloc];
}


@end
