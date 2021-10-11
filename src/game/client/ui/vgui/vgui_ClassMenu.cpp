//=========== (C) Copyright 1996-2002 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: TFC Class Menu 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "VGUI_Font.h"
#include <VGUI_TextImage.h>

#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "camera.h"
#include "in_defs.h"
#include "parsemsg.h"

#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_StatsMenuPanel.h"

// Class Menu Dimensions
#define CLASSMENU_TITLE_X				XRES(40)
#define CLASSMENU_TITLE_Y				YRES(32)
#define CLASSMENU_TOPLEFT_BUTTON_X		XRES(40)
#define CLASSMENU_TOPLEFT_BUTTON_Y		YRES(80)
#define CLASSMENU_BUTTON_SIZE_X			XRES(124)
#define CLASSMENU_BUTTON_SIZE_Y			YRES(24)
#define CLASSMENU_BUTTON_SPACER_Y		YRES(8)
#define CLASSMENU_WINDOW_X				XRES(176)
#define CLASSMENU_WINDOW_Y				YRES(80)
#define CLASSMENU_WINDOW_SIZE_X			XRES(424)
#define CLASSMENU_WINDOW_SIZE_Y			YRES(312)
#define CLASSMENU_WINDOW_TEXT_X			XRES(150)
#define CLASSMENU_WINDOW_TEXT_Y			YRES(80)
#define CLASSMENU_WINDOW_NAME_X			XRES(150)
#define CLASSMENU_WINDOW_NAME_Y			YRES(8)
#define CLASSMENU_WINDOW_PLAYERS_Y		YRES(42)

// Creation
CClassMenuPanel::CClassMenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	// don't show class graphics at below 640x480 resolution
	bool bShowClassGraphic = true;
	if ( ScreenWidth < 640 )
	{
		bShowClassGraphic = false;
	}

	memset( m_pClassImages, 0, sizeof(m_pClassImages) );

	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hClassWindowText = pSchemes->getSchemeHandle( "Briefing Text" );

	// color schemes
	int r, g, b, a;

	// Create the title
	Label *pLabel = new Label( "", CLASSMENU_TITLE_X, CLASSMENU_TITLE_Y );
	pLabel->setParent( this );
	pLabel->setFont( pSchemes->getFont(hTitleScheme) );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	pLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	pLabel->setBgColor( r, g, b, a );
	pLabel->setContentAlignment( vgui::Label::a_west );
	pLabel->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#CTFTitle_SelectYourCharacter"));

	// Create the Scroll panel
	m_pScrollPanel = new CTFScrollPanel( CLASSMENU_WINDOW_X, CLASSMENU_WINDOW_Y, CLASSMENU_WINDOW_SIZE_X, CLASSMENU_WINDOW_SIZE_Y );
	m_pScrollPanel->setParent(this);
	//force the scrollbars on, so after the validate clientClip will be smaller
	m_pScrollPanel->setScrollBarAutoVisible(false, false);
	m_pScrollPanel->setScrollBarVisible(true, true);
	m_pScrollPanel->setBorder( new LineBorder( Color(255 * 0.7,170 * 0.7,0,0) ) );
	m_pScrollPanel->validate();

	int clientWide=m_pScrollPanel->getClient()->getWide();

	//turn scrollpanel back into auto show scrollbar mode and validate
	m_pScrollPanel->setScrollBarAutoVisible(false,true);
	m_pScrollPanel->setScrollBarVisible(false,false);
	m_pScrollPanel->validate();

	// Create the Class buttons
	for( int team = 0; team < PC_MAX_TEAMS; ++team )
	{
		for( int i = 0; i < PC_RANDOM; i++ )
		{
			char sz[ 256 ];
			int iYPos = CLASSMENU_TOPLEFT_BUTTON_Y + ( ( CLASSMENU_BUTTON_SIZE_Y + CLASSMENU_BUTTON_SPACER_Y ) * i );

			sprintf( sz, "selectchar %d", i + 1 );
			ActionSignal *pASignal = new CMenuHandler_StringCommandClassSelect( sz, true );

			// Class button
			strcpy( sz, CHudTextMessage::BufferedLocaliseTextString( sLocalisedClasses[ team ][ i ] ) );
			m_pButtons[ team ][ i ] = new ClassButton( i, sz, CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );

			sprintf( sz, "%d", i + 1 );

			m_pButtons[ team ][ i ]->setBoundKey( sz[ 0 ] );
			m_pButtons[ team ][ i ]->setContentAlignment( vgui::Label::a_west );
			m_pButtons[ team ][ i ]->addActionSignal( pASignal );
			m_pButtons[ team ][ i ]->addInputSignal( new CHandler_MenuButtonOver( this, i ) );
			m_pButtons[ team ][ i ]->setParent( this );

			// Create the Class Info Window
			//m_pClassInfoPanel[i] = new CTransparentPanel( 255, CLASSMENU_WINDOW_X, CLASSMENU_WINDOW_Y, CLASSMENU_WINDOW_SIZE_X, CLASSMENU_WINDOW_SIZE_Y );
			m_pClassInfoPanel[ team ][ i ] = new CTransparentPanel( 255, 0, 0, clientWide, CLASSMENU_WINDOW_SIZE_Y );
			m_pClassInfoPanel[ team ][ i ]->setParent( m_pScrollPanel->getClient() );
			//m_pClassInfoPanel[i]->setVisible( false );

			// don't show class pic in lower resolutions
			int textOffs = XRES( 8 );

			if( bShowClassGraphic )
			{
				textOffs = CLASSMENU_WINDOW_NAME_X;
			}

			// Create the Class Name Label
			sprintf( sz, "#CTFTitle_%s", sCTFClassSelection[ team ][ i ] );
			char* localName = CHudTextMessage::BufferedLocaliseTextString( sz );
			Label *pNameLabel = new Label( "", textOffs, CLASSMENU_WINDOW_NAME_Y );
			pNameLabel->setFont( pSchemes->getFont( hTitleScheme ) );
			pNameLabel->setParent( m_pClassInfoPanel[ team ][ i ] );
			pSchemes->getFgColor( hTitleScheme, r, g, b, a );
			pNameLabel->setFgColor( r, g, b, a );
			pSchemes->getBgColor( hTitleScheme, r, g, b, a );
			pNameLabel->setBgColor( r, g, b, a );
			pNameLabel->setContentAlignment( vgui::Label::a_west );
			//pNameLabel->setBorder(new LineBorder());
			pNameLabel->setText( "%s", localName );

			// Create the Class Image
			if( bShowClassGraphic )
			{
				sprintf( sz, "%s", sCTFClassSelection[ team ][ i ] );

				m_pClassImages[ team ][ i ] = new CImageLabel( sz, 0, 0, CLASSMENU_WINDOW_TEXT_X, CLASSMENU_WINDOW_TEXT_Y );

				CImageLabel *pLabel = m_pClassImages[ team ][ i ];
				pLabel->setParent( m_pClassInfoPanel[ team ][ i ] );
				//pLabel->setBorder(new LineBorder());

				pLabel->setVisible( false );

				// Reposition it based upon it's size
				int xOut, yOut;
				pNameLabel->getTextSize( xOut, yOut );
				pLabel->setPos( ( CLASSMENU_WINDOW_TEXT_X - pLabel->getWide() ) / 2, yOut / 2 );
			}

			// Open up the Class Briefing File
			sprintf( sz, "classes/short_%s.txt", sCTFClassSelection[ team ][ i ] );
			char *cText = "Class Description not available.";
			char *pfile = ( char * ) gEngfuncs.COM_LoadFile( sz, 5, NULL );
			if( pfile )
			{
				cText = pfile;
			}

			// Create the Text info window
			TextPanel *pTextWindow = new TextPanel( cText, textOffs, CLASSMENU_WINDOW_TEXT_Y, ( CLASSMENU_WINDOW_SIZE_X - textOffs ) - 5, CLASSMENU_WINDOW_SIZE_Y - CLASSMENU_WINDOW_TEXT_Y );
			pTextWindow->setParent( m_pClassInfoPanel[ team ][ i ] );
			pTextWindow->setFont( pSchemes->getFont( hClassWindowText ) );
			pSchemes->getFgColor( hClassWindowText, r, g, b, a );
			pTextWindow->setFgColor( r, g, b, a );
			pSchemes->getBgColor( hClassWindowText, r, g, b, a );
			pTextWindow->setBgColor( r, g, b, a );

			// Resize the Info panel to fit it all
			int wide, tall;
			pTextWindow->getTextImage()->getTextSizeWrapped( wide, tall );
			pTextWindow->setSize( wide, tall );

			int xx, yy;
			pTextWindow->getPos( xx, yy );
			int maxX = xx + wide;
			int maxY = yy + tall;

			//check to see if the image goes lower than the text
			//just use the red teams [0] images
			if( m_pClassImages[ 0 ][ i ] != null )
			{
				m_pClassImages[ 0 ][ i ]->getPos( xx, yy );
				if( ( yy + m_pClassImages[ 0 ][ i ]->getTall() ) > maxY )
				{
					maxY = yy + m_pClassImages[ 0 ][ i ]->getTall();
				}
			}

			m_pClassInfoPanel[ team ][ i ]->setSize( maxX, maxY );
			if( pfile ) gEngfuncs.COM_FreeFile( pfile );
			//m_pClassInfoPanel[i]->setBorder(new LineBorder());

		}
	}

	// Create the Cancel button
	m_pCancelButton = new CommandButton( gHUD.m_TextMessage.BufferedLocaliseTextString( "#CTFMenu_Cancel" ), CLASSMENU_TOPLEFT_BUTTON_X, 0, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y);
	m_pCancelButton->setParent( this );
	m_pCancelButton->addActionSignal( new CMenuHandler_TextWindow(HIDE_TEXTWINDOW) );
	m_pCancelButton->addActionSignal( new CMenuHandler_StringCommandWatch( "cancelmenu", true ) );

	m_iCurrentInfo = 0;

}


// Update
void CClassMenuPanel::Update()
{
	// Don't allow the player to join a team if they're not in a team
	if (!gViewPort->m_iCTFTeamNumber )
		return;

	const auto teamIndex = gViewPort->m_iCTFTeamNumber - 1;

	int	 iYPos = CLASSMENU_TOPLEFT_BUTTON_Y;

	// Cycle through the rest of the buttons
	for( int team = 0; team < PC_MAX_TEAMS; ++team )
	{
		for( int i = 0; i < PC_RANDOM; i++ )
		{
			if( team != teamIndex )
			{
				m_pButtons[ team ][ i ]->setVisible( false );
			}
			else
			{
				m_pButtons[ team ][ i ]->setVisible( true );
				m_pButtons[ team ][ i ]->setPos( CLASSMENU_TOPLEFT_BUTTON_X, iYPos );
				iYPos += CLASSMENU_BUTTON_SIZE_Y + CLASSMENU_BUTTON_SPACER_Y;

				// Start with the first option up
				if( !m_iCurrentInfo )
					SetActiveInfo( i );
			}

			if (m_pClassImages[team][i])
			{
				m_pClassImages[team][i]->setVisible(true);
			}
		}
	}

	m_pCancelButton->setPos( CLASSMENU_TOPLEFT_BUTTON_X, iYPos );
	m_pCancelButton->setVisible( true );
}

//======================================
// Key inputs for the Class Menu
bool CClassMenuPanel::SlotInput( int iSlot )
{
	if ((iSlot <= 0) || (iSlot > PC_LASTCLASS))
		return false;

	//TODO: apparently bugged in vanilla, still uses old indexing code with no second array index
	ClassButton* button = m_pButtons[gViewPort->m_iCTFTeamNumber - 1][iSlot - 1];

	if (!button)
		return false;

	// Is the button pushable?
	if (!(button->IsNotValid()))
	{
		button->fireActionSignal();
		return true;
	}

	return false;
}

//======================================
// Update the Class menu before opening it
void CClassMenuPanel::Open()
{
	Update();
	CMenuPanel::Open();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CClassMenuPanel::Initialize()
{
	setVisible( false );
	m_pScrollPanel->setScrollValue( 0, 0 );
}

//======================================
// Mouse is over a class button, bring up the class info
void CClassMenuPanel::SetActiveInfo( int iInput )
{
	// Remove all the Info panels and bring up the specified one
	for( int team = 0; team < PC_MAX_TEAMS; ++team )
	{
		for( int i = 0; i < PC_RANDOM; i++ )
		{
			m_pButtons[ team ][ i ]->setArmed( false );
			m_pClassInfoPanel[ team ][ i ]->setVisible( false );
		}
	}

	if (iInput >= PC_LASTCLASS || iInput < 0)
	{
		iInput = 0;
	}

	m_pButtons[ gViewPort->m_iCTFTeamNumber - 1 ][iInput]->setArmed( true );
	m_pClassInfoPanel[ gViewPort->m_iCTFTeamNumber - 1 ][iInput]->setVisible( true );

	if( m_iCurrentInfo != iInput )
	{
		auto szImage = sCTFClassSelection[gViewPort->m_iCTFTeamNumber - 1][ iInput ];

		gViewPort->m_pStatsMenu->SetPlayerImage(szImage);
	}

	m_iCurrentInfo = iInput;

	m_pScrollPanel->setScrollValue(0,0);
	m_pScrollPanel->validate();
}

