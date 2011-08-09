//
//  DownloaderDetailViewController.h
//  Downloader
//
//  Created by Nick Geoghegan on 09/08/2011.
//  Copyright 2011 Navit Project. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface DownloaderDetailViewController : UIViewController {

@private
	UITextField *nameTextField_;
	NSDictionary *locationName_;

}

@property (nonatomic, retain) IBOutlet UITextField *nameTextField;
@property(nonatomic, retain) NSDictionary *locationName;

@end
