/*****************************************************
File: frontend.h
Description: container class for data
Author: Slash/Byte
Date: 11-30-2018
Notes: Work in progress

MIT License (https://opensource.org/licenses/MIT)
.▄▄ · ▄▄▌   ▄▄▄· .▄▄ ·  ▄ .▄▄▄▄▄·  ▄· ▄▌▄▄▄▄▄▄▄▄ .
▐█ ▀. ██•  ▐█ ▀█ ▐█ ▀. ██▪▐█▐█ ▀█▪▐█▪██▌•██  ▀▄.▀·
▄▀▀▀█▄██▪  ▄█▀▀█ ▄▀▀▀█▄██▀▐█▐█▀▀█▄▐█▌▐█▪ ▐█.▪▐▀▀▪▄
▐█▄▪▐█▐█▌▐▌▐█ ▪▐▌▐█▄▪▐███▌▐▀██▄▪▐█ ▐█▀·. ▐█▌·▐█▄▄▌
 ▀▀▀▀ .▀▀▀  ▀  ▀  ▀▀▀▀ ▀▀▀ ··▀▀▀▀   ▀ •  ▀▀▀  ▀▀▀
*****************************************************/
#include <thread>
#include <string>
#include "pstream.h"
#include "ht16k33.h"
#include "starburst.h"
#include "container.h"
#include "backend.h"
#include "frontend.h"
/************************************
This class contains functions that
manage all of the game "functions".
ie. new game, undo, move

In addition to managing the "backend"
thread functions.

This is an extension of
the "backend" class
*************************************/


/* begins the threads */
void FrontEnd::begin(void)
{
	/* start the threads */
    t1 = std::thread(&FrontEnd::readEngine, this);
    t2 = std::thread(&FrontEnd::writeEngine, this);
    t3 = std::thread(&FrontEnd::readButton, this);
    t4 = std::thread(&FrontEnd::writeDisplay, this);
	/* detach and run in the background */
    t1.detach(); t2.detach(); t3.detach(); t4.detach();
}

/* returns true if a button is pressed */
bool FrontEnd::is_button(void)
{
    return buttonOutput.is();
}

/* returns the result of the button press */
int FrontEnd::get_button(void)
{
    return buttonOutput.get_int();
}

/* returns true is there is something from the engine */
bool FrontEnd::is_engine(void)
{
    return engineOutput.is();
}

/* grabs the engine string */
std::string FrontEnd::get_engine(void)
{
    return engineOutput.get_str();
}

/* send a string to the hardware display */
void FrontEnd::send_display(std::string a)
{
    displayInput.set_str_easy(a);
}

/* 
 sends a string to the hardware display
 b = 1 uppercase, b = 0 lowercase
 */
void FrontEnd::send_display(std::string a, bool b)
{
    if(b)
		strToUpper(a);
    else 
		strToLower(a);
    displayInput.set_str_easy(a);
}

/* send a string to the engine */
void FrontEnd::send_engine(std::string a)
{
    engineInput.set_str_easy(a);
}

/* converts a string to uppercase */
void FrontEnd::strToUpper(std::string &a)
{
    for(std::string::size_type i = 0; i < a.length(); i++)
        if((a.at(i) >= 97) && (a.at(i) <= 122)) a.at(i) = a.at(i)-32;
}

/* converts a sting to lowercase */
void FrontEnd::strToLower(std::string &a)
{
    for(std::string::size_type i = 0; i < a.length(); i++)
        if((a.at(i) >= 65) && (a.at(i) <= 90)) a.at(i) = a.at(i)+32;
}

/* converts a button code to a char */
char FrontEnd::buttonToChar(int a, int b) //yeah I could have used math...
{
    if(b%2)
		return num[a-8];
    else 
		return alfa[a-8];
}
/****************************************************/
/*             Start of game play functions         */
/****************************************************/

/* undo the computers last move and your last move */
void FrontEnd::undoMove(void)
{
	while(displayBusy());  //hardwareDisplay lock check
	saveDisplayState();
    send_engine("remove"); //send undo command to the engine
	send_display("Undo");  //show the user
	hardwareDisplay->delay(500); //wait before going back to the screen
	restoreDisplayState(); 
}

/* invoke the engine */
void FrontEnd::go(void)
{
    send_engine("go"); //send command
}

/* 
 start a new game
 pass in the search depth and ponder mode
 */
void FrontEnd::newGame(int sd, bool p) //new game
{
    std::string _sd = "sd " + std::to_string(sd); //convert search depth to string
    send_engine("new");      //send the engine a command
    send_engine(_sd);
	if(p)                    //send ponder
		send_engine("hard"); //ponder on
	else
		send_engine("easy"); //ponder off
    send_display("New ");    //tell the user
}

/* 
 turns on ponder
 think on opponents move
 uses the CPU 100% of the time
 */
void FrontEnd::ponder(bool &p)
{
	while(displayBusy()); //hardwareDisplay lock check
	saveDisplayState();
	p = !p;
	if(p)
	{
		send_engine("hard"); //ponder on
		send_display("On  ");
	}
	else
	{
		send_engine("easy"); //ponder off
		send_display("Off ");
	}
	hardwareDisplay->delay(500);   //delay so the user can read
	restoreDisplayState();
}

/* 
 get a users move, pass in the first button pressed
 */
bool FrontEnd::getMove(int b)
{
    std::string move = "----";
    move.at(0) = buttonToChar(b, 0); //convert to indexed char
    int digit = 1;
    send_display(move, 1); //update with first button press, upper
    while(1) //input loop
    {
        bool _b = is_button(); //while buttons !clear or !enter, grab user input
        if(_b)
        {
            int _c = get_button();
            if((_c >= 8) && (_c <= 15) && (digit < 4)) //buttons 8-15
            {
                move.at(digit) = buttonToChar(_c, digit);
                digit++;
                send_display(move, 1); //send to display, update, upper
            }
            if(_c == 2) //clear
            {
                send_display("----"); //send to display, update
                return 1;
            }
            if(_c == 3) //enter
            {
                send_engine(move); //send move to engine
                return 0;
            }
        }
        cpuBreak();
    }
    return 1;
}

/* show the computers move */
void FrontEnd::showMove(std::string b)
{
    send_display(b, 1); //send to display, upper
}

/* show the computers hint */
void FrontEnd::showHint(std::string b)
{
	while(displayBusy()); //hardwareDisplay lock check
	saveDisplayState();
    send_display(b); //send to display
	hardwareDisplay->delay(1000); //second delay for hint display
	restoreDisplayState();
}

/* clear the display */
void FrontEnd::clear(void) //clear display, fix later
{
    send_display("----"); //would blank be better?
}

/* request FEN position verification */
void FrontEnd::posVer(void)
{
    send_engine("FEN"); //hacked polyglot command
}

/* request current board */
void FrontEnd::getBoard(void)
{
	send_engine("BOARD"); //hacked polyglot command
}

/* displays the board that polyglot sends back */
/*<<<<<<<<<<<<< used for console debugging >>>>>>>>>>>*/
void FrontEnd::printBoard(std::string a)
{
	std::string::size_type _l = a.length();
	if(_l == 17) //expected length
	{
		//magic numbers mean ice cream
		for(int i = 9; i < 17; i++)
			printf("%c ", a.at(i));
		printf("\n");
	}
	else
		printf("---------------\n");
}
/*<<<<<<<<<<<<< used for console debugging >>>>>>>>>>>*/

/* hint request */
void FrontEnd::hint(void)
{
    send_engine("hint"); //hint command
}

/* display error screen */
void FrontEnd::error(void)
{
	while(displayBusy()); //hardwareDisplay lock check
    send_display("____"); //clear display
    hardwareDisplay->setBlinkRate(3); //blink display
    bool _b;
    do
    {
        _b = is_button();
        cpuBreak();
    }
    while(!_b);  //wait for any-key to be pushed
    hardwareDisplay->setBlinkRate(0); //turn off blink
}

/* set or clear "check" LED */
void FrontEnd::check(bool b) //set=1, clr=0
{
	while(displayBusy()); //hardwareDisplay lock check
    if(b)
		hardwareDisplay->setLed(14); //fix led num
    else
        hardwareDisplay->clrLed(14); //fix led num
}

/* set or clear "mate" LED */
void FrontEnd::mate(bool b) //set=1, clr=0
{
	while(displayBusy()); //hardwareDisplay lock check
    if(b)
        hardwareDisplay->setLed(62); //fix led num
    else
        hardwareDisplay->clrLed(62); //fix led num
}

/* flash when mate (the end of the game)*/
void FrontEnd::mateBlink(void)
{
	while(displayBusy()); //hardwareDisplay lock check
    hardwareDisplay->setBlinkRate(2); //blink display
    bool _b;
    do
    {
        _b = is_button();
        cpuBreak();
    }
    while(!_b);  //wait for any-key to be pushed
    hardwareDisplay->setBlinkRate(0); //turn off blink
}

/* think screen with animation */
void FrontEnd::thinkANI(void)
{
	while(displayBusy()); //hardwareDisplay lock check
    bool _b;
    int i = 0;
    do
    {
        for(int j = 0; j < 4; j++)
            hardwareDisplay->printM(spinner.at(i),j); //load membuf
        hardwareDisplay->update(); //show on display
        i=(i+1)%4;
        hardwareDisplay->delay(100); //!!affects update time!!
		_b = is_engine() | is_button(); //break if engine or button
    }
    while(!_b);  //wait for any-key to be pushed
}

/* generic think screen, no delay in move processing */
void FrontEnd::think(void)
{
    send_display("****");
}

/* 
 display kill-screen
 if the engine gives up, display why
 pass in the message number and the scroll delay time in ms
 */
void FrontEnd::killScreen(int b, int _delay)
{
	while(displayBusy()); //hardwareDisplay lock check
	saveDisplayState();
    std::string temp = "    " + KS[b-5];
    int _tempL = temp.length();
    int i = 0;
    bool _b;
    do
    {
        for(int j = 0; j < 4; j++)
            hardwareDisplay->printM(temp.at((i+j)%_tempL), j); // writes to "display buffer"
        hardwareDisplay->update(); //pushes to display
        i=(i+1)%_tempL;
        hardwareDisplay->delay(_delay); //150ms delay, affects update time, might remove
        _b = is_button();
    }
    while(!_b);  //wait for any-key to be pushed
	restoreDisplayState();
}

/* save the current display state to a buffer*/
void FrontEnd::saveDisplayState(void)
{
	while(displayBusy()); //hardwareDisplay lock check
    for(int i = 0; i < 8; i++) //save display state
        mState[i] = hardwareDisplay->memory[i];
}

/* restore the last display state from a buffer */
void FrontEnd::restoreDisplayState(void)
{
	while(displayBusy()); //hardwareDisplay lock check
    for(int i = 0; i < 8; i++)
        hardwareDisplay->memory[i] = mState[i]; //renew state
    hardwareDisplay->update(); //display saved state
}

/* display the fen string on the hardware display */
void FrontEnd::fen(std::string b)
{
	while(displayBusy()); //hardwareDisplay lock check
    saveDisplayState();
    while(b.length()%4) //stretch to multiple of 4
        b += " ";
    std::string::size_type i = 0;
    send_display("FEN-");
    while(1)
    {
        bool _b = is_button();
        if(_b)
        {
            int _c = get_button();
            if((_c == 3) || (_c == 2)) //enter or clear, return to play
            {
                restoreDisplayState();
                return;
            }
            if(_c == 7) //pv, advance by 4
            {
                if(i < b.length())
                {
                    for(int j = 0; j < 4; j++)
                        hardwareDisplay->printM(b.at(i+j),j);
                    hardwareDisplay->update();
                    i=i+4;
                }
                else
					send_display("-END");
            }
        }
        cpuBreak();
    }
}

/* set the engine level */
void FrontEnd::level(int &sd) //set level
{
    saveDisplayState();
    std::string _sd = "Lv";
    if(sd < 10)
        _sd += "0" + std::to_string(sd);
    else
        _sd += std::to_string(sd);
    send_display(_sd);
    while(1)
    {
        bool _b = is_button();
        if(_b)
        {
            int _c = get_button();
			if(_c == 2) //clear
			{
				restoreDisplayState();
                return;
			}
            if(_c == 3) //enter
            {
                send_engine("sd " + std::to_string(sd)); //set new level
                restoreDisplayState();
                return;
            }
            if(_c == 4) //lv button, increment
            {
                sd = 1+(sd%25);
                if(sd < 10)
                    _sd = "Lv0" + std::to_string(sd);
                else
                    _sd = "Lv" + std::to_string(sd);
                send_display(_sd);
            }
        }
        cpuBreak();
    }
}