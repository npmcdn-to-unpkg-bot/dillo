#ifndef __CSS_HH__
#define __CSS_HH__

#include "dw/core.hh"
#include "doctree.hh"

/* Origin and weight. Used only internally.*/
typedef enum {
   CSS_PRIMARY_USER_AGENT,
   CSS_PRIMARY_USER,
   CSS_PRIMARY_AUTHOR,
   CSS_PRIMARY_AUTHOR_IMPORTANT,
   CSS_PRIMARY_USER_IMPORTANT,
   CSS_PRIMARY_LAST,
} CssPrimaryOrder;

class CssProperty {
   public:
      typedef union {
         int intVal;
         const char *strVal;
      } Value;

      typedef enum {
         CSS_PROPERTY_BACKGROUND_ATTACHMENT,
         CSS_PROPERTY_BACKGROUND_COLOR,
         CSS_PROPERTY_BACKGROUND_IMAGE,
         CSS_PROPERTY_BACKGROUND_POSITION,
         CSS_PROPERTY_BACKGROUND_REPEAT,
         CSS_PROPERTY_BORDER_BOTTOM_COLOR,
         CSS_PROPERTY_BORDER_BOTTOM_STYLE,
         CSS_PROPERTY_BORDER_BOTTOM_WIDTH,
         CSS_PROPERTY_BORDER_COLLAPSE,
         CSS_PROPERTY_BORDER_COLOR,
         CSS_PROPERTY_BORDER_LEFT_COLOR,
         CSS_PROPERTY_BORDER_LEFT_STYLE,
         CSS_PROPERTY_BORDER_LEFT_WIDTH,
         CSS_PROPERTY_BORDER_RIGHT_COLOR,
         CSS_PROPERTY_BORDER_RIGHT_STYLE,
         CSS_PROPERTY_BORDER_RIGHT_WIDTH,
         CSS_PROPERTY_BORDER_SPACING,
         CSS_PROPERTY_BORDER_STYLE,
         CSS_PROPERTY_BORDER_TOP_COLOR,
         CSS_PROPERTY_BORDER_TOP_STYLE,
         CSS_PROPERTY_BORDER_TOP_WIDTH,
         CSS_PROPERTY_BORDER_WIDTH,
         CSS_PROPERTY_BOTTOM,
         CSS_PROPERTY_CAPTION_SIDE,
         CSS_PROPERTY_CLEAR,
         CSS_PROPERTY_CLIP,
         CSS_PROPERTY_COLOR,
         CSS_PROPERTY_CONTENT,
         CSS_PROPERTY_COUNTER_INCREMENT,
         CSS_PROPERTY_COUNTER_RESET,
         CSS_PROPERTY_CURSOR,
         CSS_PROPERTY_DIRECTION,
         CSS_PROPERTY_DISPLAY,
         CSS_PROPERTY_EMPTY_CELLS,
         CSS_PROPERTY_FLOAT,
         CSS_PROPERTY_FONT_FAMILY,
         CSS_PROPERTY_FONT_SIZE,
         CSS_PROPERTY_FONT_SIZE_ADJUST,
         CSS_PROPERTY_FONT_STRETCH,
         CSS_PROPERTY_FONT_STYLE,
         CSS_PROPERTY_FONT_VARIANT,
         CSS_PROPERTY_FONT_WEIGHT,
         CSS_PROPERTY_HEIGHT,
         CSS_PROPERTY_LEFT,
         CSS_PROPERTY_LETTER_SPACING,
         CSS_PROPERTY_LINE_HEIGHT,
         CSS_PROPERTY_LIST_STYLE_IMAGE,
         CSS_PROPERTY_LIST_STYLE_POSITION,
         CSS_PROPERTY_LIST_STYLE_TYPE,
         CSS_PROPERTY_MARGIN,
         CSS_PROPERTY_MARGIN_BOTTOM,
         CSS_PROPERTY_MARGIN_LEFT,
         CSS_PROPERTY_MARGIN_RIGHT,
         CSS_PROPERTY_MARGIN_TOP,
         CSS_PROPERTY_MARKER_OFFSET,
         CSS_PROPERTY_MARKS,
         CSS_PROPERTY_MAX_HEIGHT,
         CSS_PROPERTY_MAX_WIDTH,
         CSS_PROPERTY_MIN_HEIGHT,
         CSS_PROPERTY_MIN_WIDTH,
         CSS_PROPERTY_OUTLINE_COLOR,
         CSS_PROPERTY_OUTLINE_STYLE,
         CSS_PROPERTY_OUTLINE_WIDTH,
         CSS_PROPERTY_OVERFLOW,
         CSS_PROPERTY_PADDING,
         CSS_PROPERTY_PADDING_BOTTOM,
         CSS_PROPERTY_PADDING_LEFT,
         CSS_PROPERTY_PADDING_RIGHT,
         CSS_PROPERTY_PADDING_TOP,
         CSS_PROPERTY_POSITION,
         CSS_PROPERTY_QUOTES,
         CSS_PROPERTY_RIGHT,
         CSS_PROPERTY_TEXT_ALIGN,
         CSS_PROPERTY_TEXT_DECORATION,
         CSS_PROPERTY_TEXT_SHADOW,
         CSS_PROPERTY_TEXT_TRANSFORM,
         CSS_PROPERTY_TOP,
         CSS_PROPERTY_UNICODE_BIDI,
         CSS_PROPERTY_VERTICAL_ALIGN,
         CSS_PROPERTY_VISIBILITY,
         CSS_PROPERTY_WHITE_SPACE,
         CSS_PROPERTY_TEXT_INDENT,
         CSS_PROPERTY_WIDTH,
         CSS_PROPERTY_WORD_SPACING,
         CSS_PROPERTY_Z_INDEX,
         CSS_PROPERTY_X_LINK,
         CSS_PROPERTY_X_COLSPAN,
         CSS_PROPERTY_X_ROWSPAN,
         PROPERTY_X_LINK,
         PROPERTY_X_IMG,
         PROPERTY_X_TOOLTIP,
         CSS_PROPERTY_LAST
      } Name;

      typedef enum {
         CSS_FONT_WEIGHT_LIGHTER = -1,
         CSS_FONT_WEIGHT_BOLDER = -2,
         CSS_FONT_WEIGHT_STEP = 300,
         /* Some special font weights. */
         CSS_FONT_WEIGHT_LIGHT = 100,
         CSS_FONT_WEIGHT_NORMAL = 400,
         CSS_FONT_WEIGHT_BOLD = 700,
         CSS_FONT_WEIGHT_MIN = 100,
         CSS_FONT_WEIGHT_MAX = 900,
      } FontWeightExtensions;

      Name name;
      Value value;

      void print ();
};

class CssPropertyList : public lout::misc::SimpleVector <CssProperty> {
      int refCount;

   public:
      CssPropertyList() : lout::misc::SimpleVector <CssProperty> (1) {
         refCount = 0;
      };
      CssPropertyList(const CssPropertyList &p) : lout::misc::SimpleVector <CssProperty> (p) {
         refCount = 0;
      };

      void set (CssProperty::Name name, CssProperty::Value value);
      void set (CssProperty::Name name, const char *value) {
         CssProperty::Value v;
         v.strVal = value;
         set (name, v);
      };
      void set (CssProperty::Name name, int value) {
         CssProperty::Value v;
         v.intVal = value;
         set (name, v);
      };
      void apply (CssPropertyList *props);
      void print ();

      inline void ref () { refCount++; }
      inline void unref () { if(--refCount == 0) delete this; }
};

/** \todo proper implementation */
class CssSelector {
   public:
      int element;
      const char *klass, *pseudo, *id;

   public:
      CssSelector (int element = -1, const char *klass = NULL,
                   const char *pseudo = NULL, const char *id = NULL) {
         this->element = element;
         this->klass = klass;
         this->pseudo = pseudo;
         this->id = id;
      };

      bool match (Doctree *dt);
      void print ();
};

class CssRule {
   private:
      CssSelector *selector;
      CssPropertyList *props;

   public:
      CssRule (CssSelector *selector, CssPropertyList *props) {
         this->selector = selector;
         this->props = props;
      };
      ~CssRule ();

      void apply (CssPropertyList *props, Doctree *docTree);
      void print ();
};

class CssStyleSheet : lout::misc::SimpleVector <CssRule*> {
   public:
      CssStyleSheet() : lout::misc::SimpleVector <CssRule*> (1) {};
      void addRule (CssRule *rule);
      void addRule (CssSelector *selector, CssPropertyList *props);
      void apply (CssPropertyList *props, Doctree *docTree);
};

class CssContext {
   private:
      static CssStyleSheet *userAgentStyle;
      static CssStyleSheet *userStyle;
      static CssStyleSheet *userImportantStyle;
      CssStyleSheet *sheet[CSS_PRIMARY_USER_IMPORTANT + 1];

      void buildUserAgentStyle ();
      void buildUserStyle ();

   public:
      CssContext ();
      ~CssContext ();

      void addRule (CssRule *rule, CssPrimaryOrder order);
      void apply (CssPropertyList *props,
         Doctree *docTree,
         CssPropertyList *tagStyle, CssPropertyList *nonCss);
};

#endif
