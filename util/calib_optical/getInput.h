/*
 *  getInput.h
 *  ARToolKit5
 *
 *  This file is part of ARToolKit.
 *
 *  ARToolKit is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ARToolKit is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2013-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#ifndef __getInput_h__
#define __getInput_h__

#ifdef __cplusplus
extern "C" {
#endif

#define INPUT_MAX_LENGTH_DEFAULT 1023
    
// Begin processing for user-supplied input.
// prompt Points to a string which will be used as the prompt.
// minLength Minimum number of characters required. Pass 0 if no minimum required.
// maxLength Maximum number of characters required. Pass 0 if no maximum specified, in which case the value INPUT_MAX_LENGTH_DEFAULT will apply.
// After completion,getInputFinish() must be called to free allocated memory.
int getInputStart(const unsigned char *prompt, unsigned int minLength, unsigned int maxLength, int intOnly, int fpOnly, int alphaOnly);

// Get the current input string. Note that unless getInputIsComplete != 0, input is not complete.
// The buffer remains valid after completion of input, up until the next call to getInputStart() or a call to getInputFinish().
// If the user has cancelled input with the ESC key, NULL will be returned.
// If the user has not entered any input, this will point to a null string.
unsigned char *getInput();

// Get the combined string consisting of the prompt, the current input, and the cursor character. This function is designed for presentation to the user.
// If the user has cancelled input with the ESC key, NULL will be returned.
unsigned char *getInputPromptAndInputAndCursor();

// If the user has pressed either ESC to cancel input, or RETURN to complete input,
// this function will return 1, otherwise 0 is returned.
// This function MAY be called on a different thread to that being used to pass
// keystrokes via getInputProcessKey().
int getInputIsComplete();

// This function waits until the user has pressed either ESC to cancel input, or RETURN to complete input.
// This function MUST be called on a different thread to that being used to pass
// keystrokes via getInputProcessKey().
void getInputWaitUntilComplete();
    
// Use this function to pass user-generated keystrokes for processing as input.
// The ASCII character set is supported.
// This function may be called on a different thread from getInputIsComplete().
void getInputProcessKey(const unsigned char keyAsciiCode);

// This function must be called once input is complete and the results are no
// longer needed, to free internally allocated memory.
void getInputFinish();

#ifdef __cplusplus
}
#endif
#endif // !__getInput_h__
