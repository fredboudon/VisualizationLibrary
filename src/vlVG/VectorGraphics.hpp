/**************************************************************************************/
/*                                                                                    */
/*  Visualization Library                                                             */
/*  http://www.visualizationlibrary.org                                               */
/*                                                                                    */
/*  Copyright (c) 2005-2010, Michele Bosi                                             */
/*  All rights reserved.                                                              */
/*                                                                                    */
/*  Redistribution and use in source and binary forms, with or without modification,  */
/*  are permitted provided that the following conditions are met:                     */
/*                                                                                    */
/*  - Redistributions of source code must retain the above copyright notice, this     */
/*  list of conditions and the following disclaimer.                                  */
/*                                                                                    */
/*  - Redistributions in binary form must reproduce the above copyright notice, this  */
/*  list of conditions and the following disclaimer in the documentation and/or       */
/*  other materials provided with the distribution.                                   */
/*                                                                                    */
/*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND   */
/*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED     */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE            */
/*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR  */
/*  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    */
/*  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;      */
/*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON    */
/*  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT           */
/*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS     */
/*  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                      */
/*                                                                                    */
/**************************************************************************************/

#ifndef VectorGraphics_INCLUDE_ONCE
#define VectorGraphics_INCLUDE_ONCE

#include <vlVG/link_config.hpp>
#include <vlCore/Image.hpp>
#include <vlCore/VisualizationLibrary.hpp>
#include <vlGraphics/Actor.hpp>
#include <vlGraphics/Text.hpp>
#include <vlGraphics/FontManager.hpp>
#include <vlGraphics/Effect.hpp>
#include <vlGraphics/SceneManager.hpp>
#include <vlGraphics/Clear.hpp>
#include <vlGraphics/Scissor.hpp>
#include <vlGraphics/Geometry.hpp>
#include <vlGraphics/FontManager.hpp>

namespace vl
{
  //! Defines how the texture is applied to the rendering primitive
  typedef enum
  {
    //! The texture is stretched over the primitive
    TextureMode_Clamp,
    //! The texture is repeated over the primitive
    TextureMode_Repeat
  } ETextureMode;

  //! Poligon stipple patterns
  typedef enum
  {
    //! The poligon is completely filled (default)
    PolygonStipple_Solid,
    PolygonStipple_Dot,
    PolygonStipple_Chain,
    PolygonStipple_HLine,
    PolygonStipple_VLine
  } EPolygonStipple;

  //! Line stipple patterns
  typedef enum
  {
    //! The line is completely filled  (default)
    LineStipple_Solid,
    LineStipple_Dot,
    LineStipple_Dash,
    LineStipple_Dash4,
    LineStipple_Dash8,
    LineStipple_DashDot,
    LineStipple_DashDotDot
  } ELineStipple;

//-------------------------------------------------------------------------------------------------------------------------------------------
// VectorGraphics
//-------------------------------------------------------------------------------------------------------------------------------------------
  /**
   * The VectorGraphics class is used in conjuction with SceneManagerVectorGraphics to generate and render 2D vector graphics.
   * The VectorGraphics object is basically nothing more than an container of Actor[s] generated by functions like
   * drawLines(), fillTriangles() etc. The Actor[s] are rendered in the order in which they are generated. 
   *
   * The VectorGraphics class features a set of advanced capabilites:
   * - Matrix transformations and matrix stack
   * - State stack
   * - All the blending operations supported by OpenGL
   * - All the stencil operations supported by OpenGL
   * - Texture mapping on all the primitives with automatic texture coordinate generation
   * - Several primitives like lines, points, quads, triangles, line strips, triangle strips, ellipses etc.
   * - Polygon and line stipple
   * - Text rendering
   * - Scissor test to clip the objects against a rectangular region
   * - Line and point smoothing
   * - Color logic operations
   *
   * For more information please refer to the \ref pagGuideVectorGraphics "2D Vector Graphics" page.
   */
  class VLVG_EXPORT VectorGraphics: public Object
  {
    VL_INSTRUMENT_CLASS(vl::VectorGraphics, Object)

  private:
    //------------------------------------------------------------------------- start internal
    //! \internal
    class ImageState
    {
    public:
      ImageState(Image* img, ETextureMode mode): mImage(img), mTextureMode(mode) {}

      bool operator<(const ImageState& other) const
      {
        if (mImage != other.mImage)
          return mImage < other.mImage;
        else
        if (mTextureMode != other.mTextureMode)
          return mTextureMode < other.mTextureMode;
        else
          return false;
      }
    protected:
      Image* mImage;
      ETextureMode mTextureMode;
    };
    //------------------------------------------------------------------------- start internal
    //! \internal
    class State
    {
    public:
      State()
      {
        mColor         = white;
        mPointSize     = 5;
        mImage         = NULL;
        mTextureMode   = TextureMode_Clamp;
        mLogicOp       = LO_COPY;
        mPointSmoothing= true;
        mLineSmoothing = true;
        mPolygonSmoothing = false;
        mLineWidth     = 1.0;
        mLineStipple   = 0xFFFF;
        memset(mPolyStipple, 0xFF, 32*32/8);

        // blend equation
        mBlendEquationRGB    = BE_FUNC_ADD;
        mBlendEquationAlpha  = BE_FUNC_ADD;
        // blend factor
        mBlendFactorSrcRGB   = BF_SRC_ALPHA;
        mBlendFactorDstRGB   = BF_ONE_MINUS_SRC_ALPHA;
        mBlendFactorSrcAlpha = BF_SRC_ALPHA;
        mBlendFactorDstAlpha = BF_ONE_MINUS_SRC_ALPHA;
        // alpha func
        mAlphaFuncRefValue = 0.0f;
        mAlphaFunc         = FU_ALWAYS;
        // font
        mFont              = defFontManager()->acquireFont("/font/bitstream-vera/VeraMono.ttf", 10, false);
        // masks
        /*mDepthMask   = true;*/
        mColorMask   = ivec4(1,1,1,1);
        // stencil
        mStencilMask = 0xFFFFFFFF;
        mStencilTestEnabled = false;
        mStencil_SFail  = SO_KEEP;
        mStencil_SFail  = SO_KEEP;
        mStencil_DpFail = SO_KEEP;
        mStencil_Function = FU_ALWAYS;
        mStencil_RefValue = 0;
        mStencil_FunctionMask = ~(unsigned int)0;
      }

      fvec4 mColor;
      int mPointSize;
      ref<Image> mImage;
      ETextureMode mTextureMode;
      ELogicOp mLogicOp;
      float mLineWidth;
      bool mPointSmoothing;
      bool mLineSmoothing;
      bool mPolygonSmoothing;
      unsigned short mLineStipple;
      unsigned char mPolyStipple[32*32/8];
      EBlendEquation mBlendEquationRGB;
      EBlendEquation mBlendEquationAlpha;
      EBlendFactor mBlendFactorSrcRGB;
      EBlendFactor mBlendFactorDstRGB;
      EBlendFactor mBlendFactorSrcAlpha;
      EBlendFactor mBlendFactorDstAlpha;
      float mAlphaFuncRefValue;
      EFunction mAlphaFunc;
      ref<Font> mFont;
      /*bool mDepthMask;*/
      ivec4 mColorMask;
      // stencil
      bool mStencilTestEnabled;
      unsigned int mStencilMask;
      EStencilOp mStencil_SFail;
      EStencilOp mStencil_DpFail;
      EStencilOp mStencil_DpPass;
      EFunction  mStencil_Function;
      int          mStencil_RefValue;
      unsigned int         mStencil_FunctionMask;

      bool operator<(const State& other) const
      {
        // lexicographic sorting
        if (mColor.r() != other.mColor.r())
          return mColor.r() < other.mColor.r();
        else
        if (mColor.g() != other.mColor.g())
          return mColor.g() < other.mColor.g();
        else
        if (mColor.b() != other.mColor.b())
          return mColor.b() < other.mColor.b();
        else
        if (mColor.a() != other.mColor.a())
          return mColor.a() < other.mColor.a();
        else
        if(mPointSize != other.mPointSize)
          return mPointSize < other.mPointSize;
        else
        if(mImage != other.mImage)
          return mImage < other.mImage;
        else
        if (mTextureMode != other.mTextureMode)
          return mTextureMode < other.mTextureMode;
        else
        if (mPolygonSmoothing != other.mPolygonSmoothing)
          return mPolygonSmoothing < other.mPolygonSmoothing;
        else
        if (mPointSmoothing!= other.mPointSmoothing)
          return mPointSmoothing < other.mPointSmoothing;
        else
        if (mLineSmoothing!= other.mLineSmoothing)
          return mLineSmoothing < other.mLineSmoothing;
        else
        if (mLineWidth != other.mLineWidth)
          return mLineWidth < other.mLineWidth;
        else
        if (mLineStipple != other.mLineStipple)
          return mLineStipple < other.mLineStipple;
        else
        if (mLogicOp != other.mLogicOp)
          return mLogicOp < other.mLogicOp;
        else
        if ( memcmp(mPolyStipple, other.mPolyStipple, 32*32/8) != 0 )
          return memcmp(mPolyStipple, other.mPolyStipple, 32*32/8) < 0;
        else
        if ( mBlendEquationRGB != other.mBlendEquationRGB)
          return mBlendEquationRGB < other.mBlendEquationRGB;
        else
        if ( mBlendEquationAlpha != other.mBlendEquationAlpha)
          return mBlendEquationAlpha < other.mBlendEquationAlpha;
        else
        if ( mBlendFactorSrcRGB != other.mBlendFactorSrcRGB)
          return mBlendFactorSrcRGB < other.mBlendFactorSrcRGB;
        else
        if ( mBlendFactorDstRGB != other.mBlendFactorDstRGB)
          return mBlendFactorDstRGB < other.mBlendFactorDstRGB;
        else
        if ( mBlendFactorSrcAlpha != other.mBlendFactorSrcAlpha)
          return mBlendFactorSrcAlpha < other.mBlendFactorSrcAlpha;
        else
        if ( mBlendFactorDstAlpha != other.mBlendFactorDstAlpha)
          return mBlendFactorDstAlpha < other.mBlendFactorDstAlpha;
        else
        if ( mAlphaFuncRefValue != other.mAlphaFuncRefValue)
          return mAlphaFuncRefValue < other.mAlphaFuncRefValue;
        else
        if ( mAlphaFunc != other.mAlphaFunc)
          return mAlphaFunc < other.mAlphaFunc;
        else
        if ( mFont != other.mFont)
          return mFont < other.mFont;
        else
        /*if ( mDepthMask != other.mDepthMask)
          return mDepthMask < other.mDepthMask;
        else*/
        if ( mColorMask.r() != other.mColorMask.r())
          return mColorMask.r() < other.mColorMask.r();
        else
        if ( mColorMask.g() != other.mColorMask.g())
          return mColorMask.g() < other.mColorMask.g();
        else
        if ( mColorMask.b() != other.mColorMask.b())
          return mColorMask.b() < other.mColorMask.b();
        else
        if ( mColorMask.a() != other.mColorMask.a())
          return mColorMask.a() < other.mColorMask.a();
        else
        if ( mStencilMask != other.mStencilMask)
          return mStencilMask < other.mStencilMask;
        else
        if ( mStencilTestEnabled != other.mStencilTestEnabled)
          return mStencilTestEnabled < other.mStencilTestEnabled;
        else
        if ( mStencil_SFail != other.mStencil_SFail )
          return mStencil_SFail < other.mStencil_SFail;
        else
        if ( mStencil_DpFail != other.mStencil_DpFail )
          return mStencil_DpFail < other.mStencil_DpFail;
        else
        if ( mStencil_DpPass != other.mStencil_DpPass )
          return mStencil_DpPass < other.mStencil_DpPass;
        else
        if ( mStencil_Function != other.mStencil_Function )
          return mStencil_Function < other.mStencil_Function;
        else
        if ( mStencil_RefValue != other.mStencil_RefValue )
          return mStencil_RefValue < other.mStencil_RefValue;
        else
        if ( mStencil_FunctionMask != other.mStencil_FunctionMask )
          return mStencil_FunctionMask < other.mStencil_FunctionMask;
        else
          return false;
      }
    };
    //------------------------------------------------------------------------- end internal

  public:    
    VectorGraphics();

    //! Returns the list of Actor[s] generated by a VectorGraphics object.
    const ActorCollection* actors() const { return &mActors; }

    //! Returns the list of Actor[s] generated by a VectorGraphics object.
    ActorCollection* actors() { return &mActors; }

    //! Renders a line starting a point <x1,y1> and ending at point <x2,y2>
    Actor* drawLine(double x1, double y1, double x2, double y2);

    //! Renders a set of lines. The 'ln' parameter shoud contain N pairs of dvec2. Each pair defines a line segment.
    Actor* drawLines(const std::vector<dvec2>& ln);

    //! Renders a line passing through the points defined by 'ln'.
    Actor* drawLineStrip(const std::vector<dvec2>& ln);

    //! Renders a closed line passing through the points defined by 'ln'.
    Actor* drawLineLoop(const std::vector<dvec2>& ln);

    //! Renders a convex polygon whose corners are defined by 'poly'
    Actor* fillPolygon(const std::vector<dvec2>& poly);

    //! Renders a set of triangles. The 'triangles' parameters must contain N triplets of dvec2. Each triplet defines a triangle.
    Actor* fillTriangles(const std::vector<dvec2>& triangles);

    //! Renders a triangle fan.
    Actor* fillTriangleFan(const std::vector<dvec2>& fan);

    //! Renders a strip of triangles as defined by the OpenGL primitive GL_TRIANGLE_STRIP.
    Actor* fillTriangleStrip(const std::vector<dvec2>& strip);

    //! Renders a set of rectangles as defined by the OpenGL primitive GL_QUADS
    Actor* fillQuads(const std::vector<dvec2>& quads);

    //! Renders a set of rectangles as defined by the OpenGL primitive GL_QUAD_STRIP
    Actor* fillQuadStrip(const std::vector<dvec2>& quad_strip);

    //! Renders a single point. This is only an utility function. If you want to draw many points use drawPoints(const std::vector<dvec2>& pt) instead.
    Actor* drawPoint(double x, double y);

    //! Renders a set of points using the currently set pointSize(), color() and image().
    Actor* drawPoints(const std::vector<dvec2>& pt);

    //! Renders the outline of an ellipse.
    Actor* drawEllipse(double origx, double origy, double xaxis, double yaxis, int segments = 64);

    //! Renders an ellipse.
    Actor* fillEllipse(double origx, double origy, double xaxis, double yaxis, int segments = 64);

    //! Utility function that renders the outline of a quad.
    Actor* drawQuad(double left, double bottom, double right, double top);

    //! Utility function that renders a single quad.
    Actor* fillQuad(double left, double bottom, double right, double top);

    /** Starts the drawing process. You have to call this function before calling any of the fill* and draw* functions.
     * This function will erase all the previously generated content of the VectorGraphics. */
    void startDrawing() { clear(); }

    /** Continues the rendering on a VectorGraphics object. This function will reset the VectorGraphics state and matrix but will not 
     * erase the previously generated graphics. */
    void continueDrawing();

    //! Ends the rendering on a VectorGraphics and releases the resources used during the Actor generation process. 
    //! If you intend to continue the rendering or to add new graphics objects later set 'release_cache' to false.
    void endDrawing(bool release_cache=true);

    //! Resets the VectorGraphics removing all the graphics objects and resetting its internal state.
    void clear();

    //! The current color. Note that the current color also modulates the currently active image.
    void setColor(const fvec4& color) { mState.mColor = color; }

    //! The current color. Note that the current color also modulates the currently active image.
    const fvec4& color() const { return mState.mColor; }

    //! The current point size
    void setPointSize(int size) { mState.mPointSize = size; }

    //! The current point size
    int pointSize() const { return mState.mPointSize; }

    //! The current image used to texture the rendered objects. Note that the current color also modulates the currently active image.
    void setImage(Image* image) { mState.mImage = image; }

    //! The current image used to texture the rendered objects. Note that the current color also modulates the currently active image.
    const Image* image() const { return mState.mImage.get(); }

    //! The current image used to texture the rendered objects. Note that the current color also modulates the currently active image.
    Image* image() { return mState.mImage.get(); }

    //! Utility function equivalent to 'setImage(image); setPointSize(image->width());'
    void setPoint(Image* image) { setImage(image); setPointSize(image->width()); }

    //! The current texture mode
    void setTextureMode(ETextureMode mode) { mState.mTextureMode = mode; }

    //! The current texture mode
    ETextureMode textureMode() const { return mState.mTextureMode; }

    //! The current logic operation, see also http://www.opengl.org/sdk/docs/man/xhtml/glLogicOp.xml for more information.
    void setLogicOp(ELogicOp op) { mState.mLogicOp = op; }

    //! The current logic operation
    ELogicOp logicOp() const { return mState.mLogicOp; }

    //! The current line width, see also http://www.opengl.org/sdk/docs/man/xhtml/glLineWidth.xml for more information.
    void setLineWidth(float width) { mState.mLineWidth = width; }

    //! The current line width
    float lineWidth() const { return mState.mLineWidth; }

    //! The current point smoothing mode
    void setPointSmoothing(bool smooth) { mState.mPointSmoothing = smooth; }

    //! The current point smoothing mode
    bool pointSmoothing() const { return mState.mPointSmoothing; }

    //! The current line smoothing mode
    void setLineSmoothing(bool smooth) { mState.mLineSmoothing = smooth; }

    //! The current line smoothing mode
    bool lineSmoothing() const { return mState.mLineSmoothing; }

    //! The current polygon smoothing mode
    void setPolygonSmoothing(bool smooth) { mState.mPolygonSmoothing = smooth; }

    //! The current polygon smoothing mode
    bool polygonSmoothing() const { return mState.mPolygonSmoothing; }

    //! The current line stipple, see also http://www.opengl.org/sdk/docs/man/xhtml/glLineStipple.xml for more information.
    void setLineStipple(ELineStipple stipple) ;

    //! The current line stipple
    void setLineStipple(unsigned short stipple) { mState.mLineStipple = stipple; }

    //! The current line stipple
    unsigned short lineStipple() const { return mState.mLineStipple; }

    //! The current polygon stipple, see also http://www.opengl.org/sdk/docs/man/xhtml/glPolygonStipple.xml for more information.
    void setPolygonStipple(EPolygonStipple stipple);

    //! The current polygon stipple
    void setPolygonStipple(unsigned char* stipple) { memcpy(mState.mPolyStipple, stipple, 32*32/8); }

    //! The current polygon stipple
    const unsigned char* polygonStipple() const { return mState.mPolyStipple; }

    //! The current polygon stipple
    unsigned char* polygonStipple() { return mState.mPolyStipple; }

    //! The current alpha function, see also http://www.opengl.org/sdk/docs/man/xhtml/glAlphaFunc.xml for more information.
    void setAlphaFunc(EFunction func, float ref_value)   { mState.mAlphaFuncRefValue=ref_value; mState.mAlphaFunc=func; }

    //! The current alpha function
    void getAlphaFunc(EFunction& func, float& ref_value) const { ref_value=mState.mAlphaFuncRefValue; func=mState.mAlphaFunc; }

    //! The current blending factor, see also http://www.opengl.org/sdk/docs/man/xhtml/glBlendFunc.xml for more information.
    void setBlendFunc(EBlendFactor src_rgb, EBlendFactor dst_rgb, EBlendFactor src_alpha, EBlendFactor dst_alpha);

    //! The current blending factor
    void getBlendFunc(EBlendFactor& src_rgb, EBlendFactor& dst_rgb, EBlendFactor& src_alpha, EBlendFactor& dst_alpha) const;

    //! The current blend equation, see also http://www.opengl.org/sdk/docs/man/xhtml/glBlendEquation.xml for more information.
    void setBlendEquation( EBlendEquation rgb_eq, EBlendEquation alpha_eq );

    //! The current blend equation.
    void getBlendEquation( EBlendEquation& rgb_eq, EBlendEquation& alpha_eq ) const;

    //! The current color mask, see also http://www.opengl.org/sdk/docs/man/xhtml/glColorMask.xml for more information.
    void setColorMask(bool r, bool g, bool b, bool a) { mState.mColorMask = ivec4(r?1:0,g?1:0,b?1:0,a?1:0); }

    //! The current color mask.
    const ivec4& colorMask() const { return mState.mColorMask; }

    /*void setDetphMask(bool mask) { mState.mDepthMask = mask; }
    bool depthMask() const { return mState.mDepthMask; }*/

    //! If set to 'true' the stencil test and operations will be enabled
    void setStencilTestEnabled(bool enabled) { mState.mStencilTestEnabled = enabled; }

    //! If set to 'true' the stencil test and operations will be enabled
    bool stencilTestEnabled() const { return mState.mStencilTestEnabled; }

    //! Current stencil mask, see also http://www.opengl.org/sdk/docs/man/xhtml/glStencilMask.xml for more information.
    void setStencilMask(unsigned int mask) { mState.mStencilMask = mask; }

    //! Current stencil mask.
    unsigned int stencilMask() const { return mState.mStencilMask; }

    //! Current stencil operation, see also http://www.opengl.org/sdk/docs/man/xhtml/glStencilOp.xml for more information.
    void setStencilOp(EStencilOp sfail, EStencilOp dpfail, EStencilOp dppass);

    //! Current stencil operation.
    void getStencilOp(EStencilOp& sfail, EStencilOp& dpfail, EStencilOp& dppass);

    //! The current stencil function, see also http://www.opengl.org/sdk/docs/man/xhtml/glStencilFunc.xml for more information.
    void setStencilFunc(EFunction func, int refval, unsigned int mask);

    //! The current stencil function.
    void getStencilFunc(EFunction& func, int& refval, unsigned int& mask);

    //! Sets the current Font
    void setFont(const String& name, int size, bool smooth=false) { mState.mFont = defFontManager()->acquireFont(name,size,smooth); }
    
    //! Sets the current Font
    void setFont(const Font* font) { setFont(font->filePath(),font->size(),font->smooth()); }
    
    //! Sets the default Font
    void setDefaultFont() { setFont(defFontManager()->acquireFont("/font/bitstream-vera/VeraMono.ttf", 10, false)); }
    
    //! Returns the current Font
    const Font* font() const { return mState.mFont.get(); }

    /** Defines the scissor box and enables the scissor test.
     * The parameters are considered in windows coordinates.
     * The Scissor is used to clip the rendering against a specific rectangular area.
     * See also http://www.opengl.org/sdk/docs/man/xhtml/glScissor.xml for more information. */
    void setScissor(int x, int y, int width, int height)
    {
      mScissor = resolveScissor(x,y,width,height);
    }

    /** Returns the currently active Scissor */
    const Scissor* scissor() const { return mScissor.get(); }

    /** Disables the Scissor test and clipping. */
    void removeScissor()
    {
      mScissor = NULL;
    }

    /** Clears the specific area of the viewport. 
     * The parameters x y w h define a rectangular area in viewport coordinates that is clipped against the viewport itself.
     * 
     * \note The specified rectangular area is not affected by the current matrix transform. */
    Actor* clearColor(const fvec4& color, int x=0, int y=0, int w=-1, int h=-1);

    /** Clears the specific area of the viewport. 
     * The parameters x y w h define a rectangular area in viewport coordinates that is clipped against the viewport itself.
     *
     * \note The specified rectangular area is not affected by the current matrix transform. */
    Actor* clearStencil(int clear_val, int x=0, int y=0, int w=-1, int h=-1);

    //! Draw the specified Text object
    Actor* drawText(Text* text);

    /** Draws the specified text at the specified position.
     * Note that the current matrix transform affect the final position, rotation and scaling of the text. */
    Actor* drawText(int x, int y, const String& text, int alignment = AlignBottom|AlignLeft);

    //! Draws the specified text
    Actor* drawText(const String& text, int alignment = AlignBottom|AlignLeft);

    /** Draws the specified Actor with the specified Transform.
     * If keep_effect is set to 'false' or the Actor's Effect is NULL a default Effect is automatically generated.
     * If 'transform' is non NULL it is bound to the Actor. */
    Actor* drawActor(Actor* actor, Transform* transform=NULL, bool keep_effect=false);

    /** Like drawActor() but instead of drawing the given actor creates a copy of it and draws that. 
     * This function is useful when you want to crate multiple instances of the same geometry. */
    Actor* drawActorCopy(Actor* actor, Transform* transform=NULL);

    //! Returns the current transform matrix
    const dmat4& matrix() const { return mMatrix; }

    //! Sets the current transform matrix
    void setMatrix(const dmat4& matrix) { mMatrix = matrix; }

    //! Resets the current transform matrix.
    void resetMatrix() { mMatrix.setIdentity(); }

    //! Performs a rotation of 'deg' degrees around the z axis.
    void rotate(double deg);

    //! Translates the current transform matrix
    void translate(double x, double y, double z=0.0);

    //! Scales the current transform matrix
    void scale(double x, double y, double z=1.0);

    //! Pushes the current matrix in the matrix stack in order to restore it later with popMatrix().
    void pushMatrix() { mMatrixStack.push_back(matrix()); }

    //! Pops the top most matrix in the matrix stack and sets it as the current matrix.
    void popMatrix();

    //! Returns the matrix stack.
    const std::vector<dmat4>& matrixStack() const { return mMatrixStack; }

    //! Pushes the current VectorGraphics state (including the matrix state) in the state stack in order to restore it later with popState().
    void pushState();

    //! Pops the top most state in the state stack and sets it as the current state.
    void popState();

    /*const std::vector<State>& stateStack() const { return mStateStack; }*/

    /** Pushes the current scissor in the scissor stack in order to restore it later with popScissor() and activates a new one.
     * The 'x', 'y', 'w' and 'h' parameters define the new scissor rectangle. 
     * Note that such rectangle is clipped against the currently active one. */
    void pushScissor(int x, int y, int w, int h);

    //! Pops the top most scissor in the scissor stack and sets it as the current scissor.
    void popScissor();

    //! Returns the scissor stack.
    const std::vector< ref<Scissor> >& scissorStack() const { return mScissorStack; }

    //! Binds the given Transform to all the Actor[s] that have been generated so far.
    void setTransform(Transform* transform) { for(int i=0; i<actors()->size(); ++i) actors()->at(i)->setTransform(transform); }

    //! Returns the Effect representing the current VectorGraphic's state.
    Effect* currentEffect() { return currentEffect(mState); }

  private:
    void generateQuadsTexCoords(Geometry* geom, const std::vector<dvec2>& points);

    void generatePlanarTexCoords(Geometry* geom, const std::vector<dvec2>& points);

    void generateLinearTexCoords(Geometry* geom);

    ref<Geometry> prepareGeometry(const std::vector<dvec2>& ln);

    ref<Geometry> prepareGeometryPolyToTriangles(const std::vector<dvec2>& ln);
  
    Scissor* resolveScissor(int x, int y, int width, int height);

    Texture* resolveTexture(Image* image);

    Effect* currentEffect(const State& vgs);

    Actor* addActor(Actor* actor) ;

  private:
    // state-machine state variables
    State mState;
    dmat4 mMatrix;
    ref<Scissor> mScissor;
    std::vector<State> mStateStack;
    std::vector<dmat4> mMatrixStack;
    std::vector< ref<Scissor> > mScissorStack;
    // state-machine state map
    std::map<State, ref<Effect> > mVGToEffectMap;
    std::map<ImageState, ref<Texture> > mImageToTextureMap;
    std::map<RectI, ref<Scissor> > mRectToScissorMap;
    ref<Effect> mDefaultEffect;
    ActorCollection mActors;
  };
//-------------------------------------------------------------------------------------------------------------------------------------------
}

#endif
