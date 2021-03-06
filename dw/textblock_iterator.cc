/*
 * Dillo Widget
 *
 * Copyright 2005-2007, 2012-2013 Sebastian Geerken <sgeerken@dillo.org>
 *
 * (Parts of this file were originally part of textblock.cc.)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "textblock.hh"
#include "../lout/msg.h"
#include "../lout/misc.hh"

#include <stdio.h>
#include <math.h>

using namespace lout;

namespace dw {

Textblock::TextblockIterator::TextblockIterator (Textblock *textblock,
                                                 core::Content::Type mask,
                                                 bool atEnd):
   core::Iterator (textblock, mask, atEnd)
{
   if (atEnd) {
      if (textblock->outOfFlowMgr) {
         oofm = true;
         index = textblock->outOfFlowMgr->getNumWidgets();
      } else {
         oofm = false;
         index = textblock->words->size();
      }
   } else {
      oofm = false;
      index = -1;
   }

   content.type = atEnd ? core::Content::END : core::Content::START;
}

Textblock::TextblockIterator::TextblockIterator (Textblock *textblock,
                                                 core::Content::Type mask,
                                                 bool oofm, int index):
   core::Iterator (textblock, mask, false)
{
   this->oofm = oofm;
   this->index = index;

   // TODO To be completely exact, oofm should be considered here.
   if (index < 0)
      content.type = core::Content::START;
   else if (index >= textblock->words->size())
      content.type = core::Content::END;
   else
      content = textblock->words->getRef(index)->content;
}

object::Object *Textblock::TextblockIterator::clone()
{
   return
      new TextblockIterator ((Textblock*)getWidget(), getMask(), oofm, index);
}

int Textblock::TextblockIterator::compareTo(object::Comparable *other)
{
   TextblockIterator *otherTI = (TextblockIterator*)other;

   if (oofm && !otherTI->oofm)
      return +1;
   else if (!oofm && otherTI->oofm)
      return -1;
   else
      return index - otherTI->index;
}

bool Textblock::TextblockIterator::next ()
{
   Textblock *textblock = (Textblock*)getWidget();

   if (content.type == core::Content::END)
      return false;

   short type;

   do {
      index++;

      if (oofm) {
         // Iterating over OOFM.
         if (index >= textblock->outOfFlowMgr->getNumWidgets()) {
            // End of OOFM list reached.
            content.type = core::Content::END;
            return false;
         }
         type = core::Content::WIDGET_OOF_CONT;
      } else {
         // Iterating over words list.
         if (index < textblock->words->size ())
            // Still words left.
            type = textblock->words->getRef(index)->content.type;
         else {
            // End of words list reached.
            if (textblock->outOfFlowMgr) {
               oofm = true;
               index = 0;
               if (textblock->outOfFlowMgr->getNumWidgets() > 0)
                  // Start with OOFM widgets.
                  type = core::Content::WIDGET_OOF_CONT;
               else {
                  // No OOFM widgets (number is 0).
                  content.type = core::Content::END;
                  return false;
               }
            } else {
               // No OOFM widgets (no OOFM agt all).
               content.type = core::Content::END;
               return false;
            }
         }
      }
   } while ((type & getMask()) == 0);

   if (oofm) {
      content.type = core::Content::WIDGET_OOF_CONT;
      content.widget = textblock->outOfFlowMgr->getWidget (index);
   } else
      content = textblock->words->getRef(index)->content;

   return true;
}

bool Textblock::TextblockIterator::prev ()
{
   Textblock *textblock = (Textblock*)getWidget();

   if (content.type == core::Content::START)
      return false;

   short type;

   do {
      index--;

      if (oofm) {
         // Iterating over OOFM.
         if (index >= 0)
            // Still widgets left.
            type = core::Content::WIDGET_OOF_CONT;
         else {
            // Beginning of OOFM list reached. Continue with words.
            oofm = false;
            index = textblock->words->size() - 1;
            if (index < 0) {
               // There are no words. (Actually, this case should not
               // happen: When there are OOF widgets, ther must be OOF
               // references, or widgets in flow, which contain
               // references.
               content.type = core::Content::END;
               return false;
            }
            type = textblock->words->getRef(index)->content.type;
         }
      } else {
         // Iterating over words list.
         if (index < 0) {
            // Beginning of words list reached.
            content.type = core::Content::START;
            return false;
         }
         type = textblock->words->getRef(index)->content.type;
      }
   } while ((type & getMask()) == 0);

   if (oofm) {
      content.type = core::Content::WIDGET_OOF_CONT;
      content.type = false;
      content.widget = textblock->outOfFlowMgr->getWidget (index);
   } else
      content = textblock->words->getRef(index)->content;

   return true;
}

void Textblock::TextblockIterator::highlight (int start, int end,
                                              core::HighlightLayer layer)
{
   if (!oofm) {
      Textblock *textblock = (Textblock*)getWidget();
      int index1 = index, index2 = index;

      int oldStartIndex = textblock->hlStart[layer].index;
      int oldStartChar = textblock->hlStart[layer].nChar;
      int oldEndIndex = textblock->hlEnd[layer].index;
      int oldEndChar = textblock->hlEnd[layer].nChar;

      if (textblock->hlStart[layer].index > textblock->hlEnd[layer].index) {
         /* nothing is highlighted */
         textblock->hlStart[layer].index = index;
         textblock->hlEnd[layer].index = index;
      }

      if (textblock->hlStart[layer].index >= index) {
         index2 = textblock->hlStart[layer].index;
         textblock->hlStart[layer].index = index;
         textblock->hlStart[layer].nChar = start;
      }

      if (textblock->hlEnd[layer].index <= index) {
         index2 = textblock->hlEnd[layer].index;
         textblock->hlEnd[layer].index = index;
         textblock->hlEnd[layer].nChar = end;
      }

      if (oldStartIndex != textblock->hlStart[layer].index ||
          oldStartChar != textblock->hlStart[layer].nChar ||
          oldEndIndex != textblock->hlEnd[layer].index ||
          oldEndChar != textblock->hlEnd[layer].nChar)
         textblock->queueDrawRange (index1, index2);
   }

   // TODO What about OOF widgets?
}

void Textblock::TextblockIterator::unhighlight (int direction,
                                                core::HighlightLayer layer)
{
   if (!oofm) {
      Textblock *textblock = (Textblock*)getWidget();
      int index1 = index, index2 = index;

      if (textblock->hlStart[layer].index > textblock->hlEnd[layer].index)
         return;

      int oldStartIndex = textblock->hlStart[layer].index;
      int oldStartChar = textblock->hlStart[layer].nChar;
      int oldEndIndex = textblock->hlEnd[layer].index;
      int oldEndChar = textblock->hlEnd[layer].nChar;

      if (direction == 0) {
         index1 = textblock->hlStart[layer].index;
         index2 = textblock->hlEnd[layer].index;
         textblock->hlStart[layer].index = 1;
         textblock->hlEnd[layer].index = 0;
      } else if (direction > 0 && textblock->hlStart[layer].index <= index) {
         index1 = textblock->hlStart[layer].index;
         textblock->hlStart[layer].index = index + 1;
         textblock->hlStart[layer].nChar = 0;
      } else if (direction < 0 && textblock->hlEnd[layer].index >= index) {
         index1 = textblock->hlEnd[layer].index;
         textblock->hlEnd[layer].index = index - 1;
         textblock->hlEnd[layer].nChar = INT_MAX;
      }

      if (oldStartIndex != textblock->hlStart[layer].index ||
          oldStartChar != textblock->hlStart[layer].nChar ||
          oldEndIndex != textblock->hlEnd[layer].index ||
          oldEndChar != textblock->hlEnd[layer].nChar)
         textblock->queueDrawRange (index1, index2);
   }

   // TODO What about OOF widgets?
}

void Textblock::TextblockIterator::getAllocation (int start, int end,
                                                  core::Allocation *allocation)
{
   Textblock *textblock = (Textblock*)getWidget();

   if (oofm) {
      // TODO Consider start and end?
      *allocation =
         *(textblock->outOfFlowMgr->getWidget(index)->getAllocation());
   } else {
      Word *word = textblock->words->getRef (index);
      int firstWordOfLine, textOffset, lineYOffsetCanvas, lineBorderAscent;

      int lineIndex = textblock->findLineOfWord (index);

      // It may be that the line does not exist yet.
      if (lineIndex != -1) {
         // Line exists: simple.
         Line *line = textblock->lines->getRef (lineIndex);
         firstWordOfLine = line->firstWord;
         textOffset = line->textOffset;
         lineYOffsetCanvas = textblock->lineYOffsetCanvas (line);
         lineBorderAscent = line->borderAscent;
      } else {        
         // Line does not exist. Calculate the values in a similar way as in
         // Textblock::addLine().
         Line *prevLine = textblock->lines->size () > 0 ?
            textblock->lines->getLastRef () : NULL;
         firstWordOfLine = prevLine ? prevLine->lastWord + 1 : 0;

         // The variable textOffset, defined below, is what Line::leftOffset
         // will be for the next line; Line::textOffset itself cannot be
         // calculated before the line is complete.
         bool regardBorder =
             textblock->mustBorderBeRegarded (textblock->lines->size ());
         textOffset =
            misc::max (regardBorder ?  textblock->newLineLeftBorder : 0,
                       textblock->boxOffsetX () +  textblock->leftInnerPadding
                       + (textblock->lines->size () == 0 ?
                             textblock->line1OffsetEff : 0));

         lineYOffsetCanvas = textblock->yOffsetOfLineToBeCreated ();

         lineBorderAscent = 0;
         for (int i = firstWordOfLine; i < textblock->words->size (); i++) {
            Word *w = textblock->words->getRef (i);
            int borderAscent =
               w->content.type == core::Content::WIDGET_IN_FLOW ?
               w->size.ascent - w->content.widget->getStyle()->margin.top :
               w->size.ascent;
            lineBorderAscent = misc::max (lineBorderAscent, borderAscent);
         }
      }

      allocation->x = textblock->allocation.x + textOffset;
      for (int i = firstWordOfLine; i < index; i++) {
         Word *w = textblock->words->getRef(i);
         allocation->x += w->size.width + w->effSpace;
      }
      if (start > 0 && word->content.type == core::Content::TEXT) {
         allocation->x += textblock->textWidth (word->content.text, 0, start,
                                                word->style,
                                                word->flags & Word::WORD_START,
                                                (word->flags & Word::WORD_END)
                                                && word->content.text[start]
                                                == 0);
      }
      allocation->y = lineYOffsetCanvas + lineBorderAscent - word->size.ascent;

      allocation->width = word->size.width;
      if (word->content.type == core::Content::TEXT) {
         int wordEnd = strlen(word->content.text);

         if (start > 0 || end < wordEnd) {
            end = misc::min(end, wordEnd); /* end could be INT_MAX */
            allocation->width =
               textblock->textWidth (word->content.text, start, end - start,
                                     word->style,
                                     (word->flags & Word::WORD_START)
                                     && start == 0,
                                     (word->flags & Word::WORD_END)
                                     && word->content.text[end] == 0);
         }
      }
      allocation->ascent = word->size.ascent;
      allocation->descent = word->size.descent;
   }
}

void Textblock::TextblockIterator::print ()
{
   Iterator::print ();
   printf (", oofm = %s, index = %d", oofm ? "true" : "false", index);

}

} // namespace dw
