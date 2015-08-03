//
//  CameraFocusView.m
//  ARApp
//
//  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
//  LLC ("Daqri") in consideration of your agreement to the following
//  terms, and your use, installation, modification or redistribution of
//  this Daqri software constitutes acceptance of these terms.  If you do
//  not agree with these terms, please do not use, install, modify or
//  redistribute this Daqri software.
//
//  In consideration of your agreement to abide by the following terms, and
//  subject to these terms, Daqri grants you a personal, non-exclusive
//  license, under Daqri's copyrights in this original Daqri software (the
//  "Daqri Software"), to use, reproduce, modify and redistribute the Daqri
//  Software, with or without modifications, in source and/or binary forms;
//  provided that if you redistribute the Daqri Software in its entirety and
//  without modifications, you must retain this notice and the following
//  text and disclaimers in all such redistributions of the Daqri Software.
//  Neither the name, trademarks, service marks or logos of Daqri LLC may
//  be used to endorse or promote products derived from the Daqri Software
//  without specific prior written permission from Daqri.  Except as
//  expressly stated in this notice, no other rights or licenses, express or
//  implied, are granted by Daqri herein, including but not limited to any
//  patent rights that may be infringed by your derivative works or by other
//  works in which the Daqri Software may be incorporated.
//
//  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
//  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
//  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
//  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
//
//  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
//  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
//  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
//  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
//  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//
//  Copyright 2015 Daqri LLC. All Rights Reserved.
//
//  Author(s): Philip Lamb
//

#import "CameraFocusView.h"

@implementation CameraFocusView {
    CABasicAnimation *_selectionBlink;
}

/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect {
    // Drawing code
}
*/

/**
 This is the init method for the square. It sets the frame for the view and sets border parameters. It also creates the blink animation.
 */
- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame]) {
        
        self.backgroundColor = [UIColor clearColor];
        self.layer.borderWidth = 1.0f;
        self.layer.borderColor = [UIColor whiteColor].CGColor;
        self.alpha = 0.0f;
        self.hidden = YES;
        
        // create the blink animation
        _selectionBlink = [[CABasicAnimation animationWithKeyPath:@"borderColor"] retain];
        _selectionBlink.toValue = (id)[UIColor clearColor].CGColor;
        _selectionBlink.repeatCount = 3;  // number of blinks
        _selectionBlink.duration = 0.4;  // this is duration per blink
        _selectionBlink.delegate = self;
    }
    return self;
}

- (void) dealloc
{
    if (_selectionBlink) {
        [_selectionBlink release];
        _selectionBlink = nil;
    }
    [super dealloc];
}

/**
 Updates the location of the view based on the incoming touchPoint.
 */
- (void)updatePoint:(CGPoint)touchPoint
{
    CGFloat squareWidth = 50.0f;
    CGRect frame = CGRectMake(touchPoint.x - squareWidth/2.0f, touchPoint.y - squareWidth/2.0f, squareWidth, squareWidth);
    self.frame = frame;
}

/**
 This unhides the view and initiates the animation by adding it to the layer.
 */
- (void)animateFocusingAction
{
    // make the view visible
    self.alpha = 1.0f;
    self.hidden = NO;
    // initiate the animation
    [self.layer addAnimation:_selectionBlink forKey:@"selectionAnimation"];
}

/**
 Hides the view after the animation stops. Since the animation is automatically removed, we don't need to do anything else here.
 */
- (void)animationDidStop:(CAAnimation *)animation finished:(BOOL)flag
{
    // hide the view
    self.alpha = 0.0f;
    self.hidden = YES;
}

@end
