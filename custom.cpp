#include "custom.hpp"

// MyDataViewListCtrl

MyDataViewListCtrl::MyDataViewListCtrl(wxWindow *parent)
    : wxDataViewListCtrl(parent, wxID_ANY) {}

// StatusRenderer

StatusRenderer::StatusRenderer()
    : wxDataViewCustomRenderer("string", wxDATAVIEW_CELL_INERT,
                               wxALIGN_CENTER) {}

bool StatusRenderer::Render(wxRect rect, wxDC *dc, int state) {
  dc->SetBrush(*wxGREEN_BRUSH);
  dc->SetPen(*wxTRANSPARENT_PEN);

  if (wxStrcmp(m_value, "OK") == 0)
    dc->SetTextForeground(*wxGREEN);
  else
    dc->SetTextForeground(*wxRED);

  RenderText(m_value, 0, rect, dc, state);
  return true;
}

bool StatusRenderer::SetValue(const wxVariant &value) {
  m_value = value.GetString();
  return true;
}

wxSize StatusRenderer::GetSize() const {
  return GetView()->FromDIP(wxSize(60, 20));
}

bool StatusRenderer::GetValue(wxVariant &WXUNUSED(value)) const {
  return true;
}
