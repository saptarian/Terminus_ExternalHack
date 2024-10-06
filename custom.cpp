#include "custom.hpp"

#include "wx/dataview.h"

// MyDataViewListCtrl

MyDataViewListCtrl::MyDataViewListCtrl(wxWindow* parent)
    : wxDataViewListCtrl(parent, wxID_ANY) {}

// StatusRenderer

StatusRenderer::StatusRenderer()
    : wxDataViewCustomRenderer("string", wxDATAVIEW_CELL_INERT,
                               wxALIGN_CENTER) {}

bool StatusRenderer::Render(wxRect rect, wxDC* dc, int state) {
  dc->SetBrush(*wxGREEN_BRUSH);
  dc->SetPen(*wxTRANSPARENT_PEN);

  if (wxStrcmp(m_value, "OK") == 0)
    dc->SetTextForeground(*wxGREEN);
  else
    dc->SetTextForeground(*wxRED);

  RenderText(m_value, 0, rect, dc, state);
  return true;
}

bool StatusRenderer::SetValue(const wxVariant& value) {
  m_value = value.GetString();
  return true;
}

wxSize StatusRenderer::GetSize() const {
  return GetView()->FromDIP(wxSize(60, 20));
}

bool StatusRenderer::GetValue(wxVariant& WXUNUSED(value)) const { return true; }

// AboutDialog

AboutDialog::AboutDialog(wxWindow* parent, wxWindowID id, const wxString& title)
    : wxDialog(parent, id, title, wxDefaultPosition, wxSize(400, 300),
               wxDEFAULT_DIALOG_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX)),
      m_title(title) {
  InitUI();
}

#include "build_count.h"

void AboutDialog::InitUI() {
  this->SetIcon(wxICON(sample));

  wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

  wxStaticText* titleText = new wxStaticText(this, wxID_ANY, m_title);
  wxStaticText* copyrightText = new wxStaticText(
      this, wxID_ANY, wxString::Format("Copyright %c 2024 @saptarian", 169));
  wxStaticText* versionText = new wxStaticText(
      this, wxID_ANY,
      wxString::Format("Stable Release, Build %d", BUILD_COUNT));
  wxStaticText* githubText =
      new wxStaticText(this, wxID_ANY, "GitHub: https://github.com/saptarian");

  // Set a larger font for the title
  wxFont titleFont = titleText->GetFont();
  titleFont.SetPointSize(16);  // Increase the font size
  titleText->SetFont(titleFont);

  // Center the title text horizontally
  mainSizer->Add(titleText, 0, wxALIGN_CENTER | wxTOP | wxLEFT | wxRIGHT, 20);

  // Add a blank space
  mainSizer->Add(0, 20);  // 20 pixels of vertical space

  // Center the three lines of text horizontally and add them close together
  mainSizer->Add(copyrightText, 0, wxALIGN_CENTER | wxTOP, 5);
  mainSizer->Add(githubText, 0, wxALIGN_CENTER | wxTOP, 5);
  mainSizer->Add(versionText, 0, wxALIGN_CENTER | wxTOP, 5);

  // Add a blank space
  mainSizer->Add(0, 20);  // 20 pixels of vertical space

  this->SetSizer(mainSizer);
  mainSizer->Fit(this);
  this->Centre();
}
