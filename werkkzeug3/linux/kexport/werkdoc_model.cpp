// This file is distributed under a BSD license. See LICENSE.txt for details.

// The document model of the werkkzeug3 editor (pages, operators, events,
// splines, loading and .kx export), compiled without the editor's GUI.
// werkdoc_model.inc is cut out of ../werkkzeug.cpp at build time
// (sections "document" and "classes").

#include "werkkzeug.hpp"
#include "kdoc.hpp"
#include "appbrowser.hpp"
#include "_viruz2.hpp"
#include "_ogg.hpp"
#include "genoverlay.hpp"
#include "genbitmap.hpp"
#include "engine.hpp"
#include "novaplayer.hpp"

#include "winpage.hpp"
#include "winview.hpp"
#include "wintimeline.hpp"
#include "winspline.hpp"
#include "winpara.hpp"
#include "wintool.hpp"
#include "wincalc.hpp"
#include "winnova.hpp"
#include "winnovaview.hpp"

#include "werkdoc_model.inc"
