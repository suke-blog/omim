#include "../base/SRC_FIRST.hpp"

#include "overlay_renderer.hpp"
#include "composite_overlay_element.hpp"
#include "straight_text_element.hpp"
#include "path_text_element.hpp"
#include "symbol_element.hpp"
#include "circle_element.hpp"
#include "render_state.hpp"
#include "info_layer.hpp"
#include "resource_manager.hpp"
#include "skin.hpp"

namespace yg
{
  namespace gl
  {
    OverlayRenderer::Params::Params()
      : m_drawTexts(true),
        m_drawSymbols(true)
    {
    }

    OverlayRenderer::OverlayRenderer(Params const & p)
      : TextRenderer(p),
        m_drawTexts(p.m_drawTexts),
        m_drawSymbols(p.m_drawSymbols),
        m_infoLayer(p.m_infoLayer)
    {
    }

    void OverlayRenderer::drawSymbol(m2::PointD const & pt,
                                     string const & symbolName,
                                     EPosition pos,
                                     int depth)
    {
      if (!m_drawSymbols)
        return;

      SymbolElement::Params params;
      params.m_depth = depth;
      params.m_position = pos;
      params.m_pivot = pt;
      params.m_symbolName = symbolName;
      params.m_skin = skin().get();

      shared_ptr<OverlayElement> oe(new SymbolElement(params));

      math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

      if (!m_infoLayer.get())
        oe->draw(this, id);
      else
        m_infoLayer->processOverlayElement(oe);
    }

    void OverlayRenderer::drawCircle(m2::PointD const & pt,
                                     yg::CircleInfo const & ci,
                                     EPosition pos,
                                     int depth)
    {
      CircleElement::Params params;

      params.m_depth = depth;
      params.m_position = pos;
      params.m_pivot = pt;
      params.m_ci = ci;

      shared_ptr<OverlayElement> oe(new CircleElement(params));

      math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

      if (!m_infoLayer.get())
        oe->draw(this, id);
      else
        m_infoLayer->processOverlayElement(oe);
    }

    void OverlayRenderer::drawText(FontDesc const & fontDesc,
                                m2::PointD const & pt,
                                yg::EPosition pos,
                                string const & utf8Text,
                                double depth,
                                bool log2vis,
                                bool doSplit)
    {
      if (!m_drawTexts)
        return;

      StraightTextElement::Params params;
      params.m_depth = depth;
      params.m_fontDesc = fontDesc;
      params.m_log2vis = log2vis;
      params.m_pivot = pt;
      params.m_position = pos;
      params.m_glyphCache = glyphCache();
      params.m_logText = strings::MakeUniString(utf8Text);
      params.m_doSplit = doSplit;

      shared_ptr<OverlayElement> oe(new StraightTextElement(params));

      math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

      if (!m_infoLayer.get())
        oe->draw(this, id);
      else
        m_infoLayer->processOverlayElement(oe);
    }

    bool OverlayRenderer::drawPathText(
      FontDesc const & fontDesc, m2::PointD const * path, size_t s, string const & utf8Text,
      double fullLength, double pathOffset, yg::EPosition pos, double depth)
    {
      if (!m_drawTexts)
        return false;

      PathTextElement::Params params;

      params.m_pts = path;
      params.m_ptsCount = s;
      params.m_fullLength = fullLength;
      params.m_pathOffset = pathOffset;
      params.m_fontDesc = fontDesc;
      params.m_logText = strings::MakeUniString(utf8Text);
      params.m_depth = depth;
      params.m_log2vis = true;
      params.m_glyphCache = glyphCache();
      params.m_pivot = path[0];
      params.m_position = pos;

      shared_ptr<PathTextElement> pte(new PathTextElement(params));

      math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

      if (!m_infoLayer.get())
        pte->draw(this, id);
      else
        m_infoLayer->processOverlayElement(pte);

      return true;
    }

    void OverlayRenderer::setInfoLayer(shared_ptr<InfoLayer> const & infoLayer)
    {
      m_infoLayer = infoLayer;
    }

    shared_ptr<InfoLayer> const & OverlayRenderer::infoLayer() const
    {
      return m_infoLayer;
    }

    void OverlayRenderer::resetInfoLayer()
    {
      m_infoLayer.reset();
    }

    void OverlayRenderer::endFrame()
    {
      if (m_infoLayer != 0)
      {
        for (TElements::const_iterator it = m_elements.begin(); it != m_elements.end(); ++it)
          m_infoLayer->processOverlayElement(it->second);
      }

      m_elements.clear();

      base_t::endFrame();
    }
  }
}
