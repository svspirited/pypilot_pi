/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  pypilot Plugin
 * Author:   Sean D'Epagnier
 *
 ***************************************************************************
 *   Copyright (C) 2018 by Sean D'Epagnier                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************
 */

#include "pypilot_pi.h"
#include "pypilotDialog.h"
#include "ConfigurationDialog.h"
#include "StatisticsDialog.h"

pypilotDialog::pypilotDialog( pypilot_pi &_pypilot_pi, wxWindow* parent)
    : pypilotDialogBase( parent ),
      m_pypilot_pi(_pypilot_pi)
{
    wxFileConfig *pConf = GetOCPNConfigObject();

    pConf->SetPath ( _T( "/Settings/pypilot" ) );

#ifdef __WXGTK__
    Move(0, 0);        // workaround for gtk autocentre dialog behavior
#endif
    Move(pConf->Read ( _T ( "DialogPosX" ), 20L ), pConf->Read ( _T ( "DialogPosY" ), 20L ));

    RebuildControlAngles();
    this->GetSizer()->Fit( this );
    this->Layout();
    this->SetSizeHints( GetSize().x, GetSize().y );
}

pypilotDialog::~pypilotDialog()
{
    wxFileConfig *pConf = GetOCPNConfigObject();
    pConf->SetPath ( _T ( "/Settings/pypilot" ) );

    wxPoint p = GetPosition();

    pConf->Write ( _T ( "DialogPosX" ), p.x );
    pConf->Write ( _T ( "DialogPosY" ), p.y );
}

void pypilotDialog::Receive(wxString &name, wxJSONValue &value)
{
    if(name == "ap.heading_command")
        m_stCommand->SetLabel(wxString::Format("%.1f", value.AsDouble()));
    else if(name == "ap.heading")
        m_stHeading->SetLabel(wxString::Format("%.1f", value.AsDouble()));
    else if(name == "ap.mode")
        m_cMode->SetStringSelection(value.AsString());
    else if(name == "ap.enabled")
        m_bAP->SetValue(value.AsBool());
}

const char **pypilotDialog::GetWatchlist()
{
    static const char *watchlist[] =
        {"ap.enabled", "ap.mode", "ap.heading", "ap.heading_command",
         "gps.source", "wind.source", "servo.flags", "servo.mode", 0};
    return watchlist;
}

void pypilotDialog::RebuildControlAngles()
{
    wxFileConfig *pConf = GetOCPNConfigObject();
    pConf->SetPath ( _T( "/Settings/pypilot" ) );
    wxString ControlAngles = pConf->Read ( _T ( "ControlAngles" ), "1;5;10;30;110;" );
    m_fgControlAnglesPos->DeleteWindows();
    m_fgControlAnglesNeg->DeleteWindows();
    while(ControlAngles.size()) {
        wxString angle = ControlAngles.BeforeFirst(';');
        long a;
        if(angle.ToLong(&a)) {
            for(int sign=-1; sign<2; sign+=2) {
                wxButton *button = new wxButton( this, wxID_ANY, wxString::Format("%ld", a*sign));
                button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( pypilotDialog::OnControlAngle ), NULL, this );
                if(sign > 0)
                    m_fgControlAnglesPos->Add( button, 0, wxALL, 5 );
                else
                    m_fgControlAnglesNeg->Add( button, 0, wxALL, 5 );
            }
        }
        ControlAngles = ControlAngles.AfterFirst(';');
    }
    this->GetSizer()->Fit(this);
    Fit();
    this->SetSize(wxSize(400,400));
}

void pypilotDialog::OnConfiguration( wxCommandEvent& event )
{
    m_pypilot_pi.m_ConfigurationDialog->Show();
    m_pypilot_pi.UpdateWatchlist();
}

void pypilotDialog::OnStatistics( wxCommandEvent& event )
{
    m_pypilot_pi.m_StatisticsDialog->Show();
    m_pypilot_pi.UpdateWatchlist();
}

void pypilotDialog::OnClose( wxCommandEvent& event )
{
    Hide();
    m_pypilot_pi.UpdateWatchlist();
}

void pypilotDialog::OnControlAngle( wxCommandEvent& event )
{
    wxButton *button = static_cast<wxButton*>(event.GetEventObject());
    wxString angle = button->GetLabel(), heading_command = m_stCommand->GetLabel();
    double a, b;
    if(heading_command.ToDouble(&a) && angle.ToDouble(&b))
        m_pypilot_pi.m_client.set("ap.heading_command", a + b);
}