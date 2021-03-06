
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "main.h"
#include "drc.h"
#include "dsp_inlines.h"
#include "geo_zlist.h"
#include "errorlog.h"


//--------------------------------------------------------------------------
// GEO configuration

namespace {
    // For trapezoid list bloating:  bloat(delta, DRC_BLOAT).  This
    // implements an external bloating algorithm.
    //
    Zlist *
    ifBloatList(const Zlist *zl, int delta, bool edgeonly, XIrt *retp)
    {
        // This method makes use of the DRC sizing function.  A temporary
        // layer is created in the current cell.
        //
        CDs *cursd = CurCell(Physical);
        if (!cursd)
            return (0);
        Zlist *z0 = 0;
        CDl *ld = Zlist::to_temp_layer(Zlist::copy(zl), DRC_TMPLYR,
            TTLinternal | TTLnoinsert | TTLjoin, cursd, retp);
        if (*retp != XIok)
            return (0);

        bool ok;
        DRCtestDesc td(0, 0, 0, 0, delta, EdgeDontCare, EdgeDontCare, &ok);
        td.setSourceLayer(ld);

        CDg gdesc;
        gdesc.init_gen(cursd, ld);
        CDo *odesc;
        Zlist *ze = 0;
        while ((odesc = gdesc.next()) != 0) {
            Zlist *ztmp;
            *retp = td.bloat(odesc, &ztmp, edgeonly);
            if (*retp != XIok) {
                Zlist::destroy(z0);
                cursd->clearLayer(ld);
                return (0);
            }
            if (!z0)
                z0 = ze = ztmp;
            else {
                while (ze->next)
                    ze = ze->next;
                ze->next = ztmp;
            }
        }
        cursd->clearLayer(ld);
        return (z0);
    }


    Zlist *
    ifBloatObj(const CDo *od, int delta, XIrt *retp)
    {
        if (!od || od->type() == CDLABEL || od->type() == CDINSTANCE) {
            *retp = XIbad;
            return (0);
        }

        bool ok;
        DRCtestDesc td(0, 0, 0, 0, delta, EdgeDontCare, EdgeDontCare, &ok);
        if (!ok) {
            *retp = XIbad;
            return (0);
        }
        td.setSourceLayer(od->ldesc());

        Zlist *ztmp;
        *retp = td.bloat(od, &ztmp);
        if (*retp != XIok)
            return (0);
        return (ztmp);
    }
}


void
cDRC::setupInterface()
{
    GEO()->RegisterIfBloatList(ifBloatList);
    GEO()->RegisterIfBloatObj(ifBloatObj);
}

