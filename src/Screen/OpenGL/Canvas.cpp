/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Screen/Canvas.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/OpenGL/Globals.hpp"
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Scope.hpp"
#include "Screen/OpenGL/Cache.hpp"
#include "Screen/OpenGL/VertexArray.hpp"
#include "Screen/OpenGL/Shapes.hpp"
#include "Screen/OpenGL/Buffer.hpp"
#include "Screen/OpenGL/Features.hpp"
#include "Screen/OpenGL/Compatibility.hpp"
#include "Screen/Util.hpp"

#include <assert.h>

AllocatedArray<RasterPoint> Canvas::vertex_buffer;

void
Canvas::fill_rectangle(PixelScalar left, PixelScalar top,
                       PixelScalar right, PixelScalar bottom,
                       const Color color)
{
  color.Set();

#ifdef HAVE_GLES
  const RasterPoint vertices[] = {
    { left, top },
    { right, top },
    { left, bottom },
    { right, bottom },
  };

  glVertexPointer(2, GL_VALUE, 0, vertices);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#else
  glRecti(left, top, right, bottom);
#endif
}

void
Canvas::OutlineRectangleGL(PixelScalar left, PixelScalar top,
                           PixelScalar right, PixelScalar bottom)
{
  const RasterPoint vertices[] = {
    { left, top },
    { right, top },
    { right, bottom },
    { left, bottom },
  };

  glVertexPointer(2, GL_VALUE, 0, vertices);
  glDrawArrays(GL_LINE_LOOP, 0, 4);
}

void
Canvas::raised_edge(PixelRect &rc)
{
  Pen bright(1, Color(240, 240, 240));
  select(bright);
  two_lines(rc.left, rc.bottom - 2, rc.left, rc.top,
            rc.right - 2, rc.top);

  Pen dark(1, Color(128, 128, 128));
  select(dark);
  two_lines(rc.left + 1, rc.bottom - 1, rc.right - 1, rc.bottom - 1,
            rc.right - 1, rc.top + 1);

  ++rc.left;
  ++rc.top;
  --rc.right;
  --rc.bottom;
}

void
Canvas::polyline(const RasterPoint *points, unsigned num_points)
{
  glVertexPointer(2, GL_VALUE, 0, points);

  pen.Set();
  glDrawArrays(GL_LINE_STRIP, 0, num_points);
}

void
Canvas::polygon(const RasterPoint *points, unsigned num_points)
{
  if (brush.IsHollow() && !pen.IsDefined())
    return;

  glVertexPointer(2, GL_VALUE, 0, points);

  if (!brush.IsHollow() && num_points >= 3) {
    brush.Set();

    static AllocatedArray<GLushort> triangle_buffer;
    unsigned idx_count = PolygonToTriangles(points, num_points,
                                            triangle_buffer);
    if (idx_count > 0)
      glDrawElements(GL_TRIANGLES, idx_count, GL_UNSIGNED_SHORT,
                     triangle_buffer.begin());
  }

  if (pen_over_brush()) {
    pen.Set();
    if (pen.GetWidth() <= 2) {
      glDrawArrays(GL_LINE_LOOP, 0, num_points);
    } else {
      unsigned vertices = LineToTriangles(points, num_points, vertex_buffer,
                                          pen.GetWidth(), true);
      if (vertices > 0) {
        glVertexPointer(2, GL_VALUE, 0, vertex_buffer.begin());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices);
      }
    }
  }
}

void
Canvas::TriangleFan(const RasterPoint *points, unsigned num_points)
{
  if (brush.IsHollow() && !pen.IsDefined())
    return;

  glVertexPointer(2, GL_VALUE, 0, points);

  if (!brush.IsHollow() && num_points >= 3) {
    brush.Set();
    glDrawArrays(GL_TRIANGLE_FAN, 0, num_points);
  }

  if (pen_over_brush()) {
    pen.Set();
    if (pen.GetWidth() <= 2) {
      glDrawArrays(GL_LINE_LOOP, 0, num_points);
    } else {
      unsigned vertices = LineToTriangles(points, num_points, vertex_buffer,
                                          pen.GetWidth(), true);
      if (vertices > 0) {
        glVertexPointer(2, GL_VALUE, 0, vertex_buffer.begin());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices);
      }
    }
  }
}

void
Canvas::line(PixelScalar ax, PixelScalar ay, PixelScalar bx, PixelScalar by)
{
  pen.Set();

  const GLvalue v[] = { ax, ay, bx, by };
  glVertexPointer(2, GL_VALUE, 0, v);
  glDrawArrays(GL_LINE_STRIP, 0, 2);
}

/**
 * Draw a line from a to b, using triangle caps if pen-size > 2 to hide
 * gaps between consecutive lines.
 */
void
Canvas::line_piece(const RasterPoint a, const RasterPoint b)
{
  pen.Set();

  const RasterPoint v[] = { {a.x, a.y}, {b.x, b.y} };
  if (pen.GetWidth() > 2) {
    unsigned strip_len = LineToTriangles(v, 2, vertex_buffer, pen.GetWidth(),
                                         false, true);
    if (strip_len > 0) {
      glVertexPointer(2, GL_VALUE, 0, vertex_buffer.begin());
      glDrawArrays(GL_TRIANGLE_STRIP, 0, strip_len);
    }
  } else {
    glVertexPointer(2, GL_VALUE, 0, &v[0].x);
    glDrawArrays(GL_LINE_STRIP, 0, 2);
  }
}

void
Canvas::line_square_piece(const RasterPoint a, const RasterPoint b)
{
  pen.set();

  const RasterPoint v[] = { {a.x, a.y}, {b.x, b.y} };
    glVertexPointer(2, GL_VALUE, 0, &v[0].x);
    glDrawArrays(GL_LINE_STRIP, 0, 2);
}

void
Canvas::two_lines(PixelScalar ax, PixelScalar ay,
                  PixelScalar bx, PixelScalar by,
                  PixelScalar cx, PixelScalar cy)
{
  pen.Set();

  const GLvalue v[] = { ax, ay, bx, by, cx, cy };
  glVertexPointer(2, GL_VALUE, 0, v);
  glDrawArrays(GL_LINE_STRIP, 0, 3);
}

void
Canvas::circle(PixelScalar x, PixelScalar y, UPixelScalar radius)
{
  if (pen_over_brush() && pen.GetWidth() > 2) {
    GLDonutVertices vertices(x, y,
                             radius - pen.GetWidth() / 2,
                             radius + pen.GetWidth() / 2);
    if (!brush.IsHollow()) {
      vertices.bind_inner_circle();
      brush.Set();
      glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.CIRCLE_SIZE);
    }
    vertices.bind();
    pen.Set();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.SIZE);
  } else if (OpenGL::vertex_buffer_object && radius < 16) {
    /* draw a "small" circle with VBO */

    OpenGL::small_circle_buffer->Bind();
    glVertexPointer(2, GL_VALUE, 0, NULL);

    glPushMatrix();

#ifdef HAVE_GLES
    glTranslatex((GLfixed)x << 16, (GLfixed)y << 16, 0);
    glScalex((GLfixed)radius << 8, (GLfixed)radius << 8, (GLfixed)1 << 16);
#else
    glTranslatef(x, y, 0.);
    glScalef(radius / 256., radius / 256., 1.);
#endif

    if (!brush.IsHollow()) {
      brush.Set();
      glDrawArrays(GL_TRIANGLE_FAN, 0, OpenGL::SMALL_CIRCLE_SIZE);
    }

    if (pen_over_brush()) {
      pen.Set();
      glDrawArrays(GL_LINE_LOOP, 0, OpenGL::SMALL_CIRCLE_SIZE);
    }

    glPopMatrix();

    OpenGL::small_circle_buffer->Unbind();
  } else if (OpenGL::vertex_buffer_object) {
    /* draw a "big" circle with VBO */

    OpenGL::circle_buffer->Bind();
    glVertexPointer(2, GL_VALUE, 0, NULL);

    glPushMatrix();

#ifdef HAVE_GLES
    glTranslatex((GLfixed)x << 16, (GLfixed)y << 16, 0);
    glScalex((GLfixed)radius << 6, (GLfixed)radius << 6, (GLfixed)1 << 16);
#else
    glTranslatef(x, y, 0.);
    glScalef(radius / 1024., radius / 1024., 1.);
#endif

    if (!brush.IsHollow()) {
      brush.Set();
      glDrawArrays(GL_TRIANGLE_FAN, 0, OpenGL::CIRCLE_SIZE);
    }

    if (pen_over_brush()) {
      pen.Set();
      glDrawArrays(GL_LINE_LOOP, 0, OpenGL::CIRCLE_SIZE);
    }

    glPopMatrix();

    OpenGL::circle_buffer->Unbind();
  } else {
    GLCircleVertices vertices(x, y, radius);
    vertices.bind();

    if (!brush.IsHollow()) {
      brush.Set();
      glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.SIZE);
    }

    if (pen_over_brush()) {
      pen.Set();
      glDrawArrays(GL_LINE_LOOP, 0, vertices.SIZE);
    }
  }
}

void
Canvas::segment(PixelScalar x, PixelScalar y, UPixelScalar radius,
                Angle start, Angle end, bool horizon)
{
  ::Segment(*this, x, y, radius, start, end, horizon);
}

gcc_const
static unsigned
AngleToDonutVertex(Angle angle)
{
  return (NATIVE_TO_INT(angle.Native()) * 64 / 4096 + 48) & 0x3e;
}

void
Canvas::annulus(PixelScalar x, PixelScalar y,
                UPixelScalar small_radius, UPixelScalar big_radius,
                Angle start, Angle end)
{
  GLDonutVertices vertices(x, y, small_radius, big_radius);

  const unsigned istart = AngleToDonutVertex(start);
  const unsigned iend = AngleToDonutVertex(end);

  if (!brush.IsHollow()) {
    brush.Set();
    vertices.bind();

    if (istart > iend) {
      glDrawArrays(GL_TRIANGLE_STRIP, istart, 64 - istart + 2);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, iend + 2);
    } else {
      glDrawArrays(GL_TRIANGLE_STRIP, istart, iend - istart + 2);
    }
  }

  if (pen_over_brush()) {
    pen.Set();

    if (istart != iend) {
      if (brush.IsHollow())
        vertices.bind();

      glDrawArrays(GL_LINE_STRIP, istart, 2);
      glDrawArrays(GL_LINE_STRIP, iend, 2);
    }

    const unsigned pstart = istart / 2;
    const unsigned pend = iend / 2;

    vertices.bind_inner_circle();
    if (pstart < pend) {
      glDrawArrays(GL_LINE_STRIP, pstart, pend - pstart + 1);
    } else {
      glDrawArrays(GL_LINE_STRIP, pstart, 32 - pstart + 1);
      glDrawArrays(GL_LINE_STRIP, 0, pend + 1);
    }

    vertices.bind_outer_circle();
    if (pstart < pend) {
      glDrawArrays(GL_LINE_STRIP, pstart, pend - pstart + 1);
    } else {
      glDrawArrays(GL_LINE_STRIP, pstart, 32 - pstart + 1);
      glDrawArrays(GL_LINE_STRIP, 0, pend + 1);
    }
  }
}

void
Canvas::keyhole(PixelScalar x, PixelScalar y,
                UPixelScalar small_radius, UPixelScalar big_radius,
                Angle start, Angle end)
{
  ::KeyHole(*this, x, y, big_radius, start, end, small_radius);
}

void
Canvas::draw_focus(PixelRect rc)
{
  outline_rectangle(rc.left, rc.top, rc.right, rc.bottom, COLOR_WHITE);
}

void
Canvas::text(PixelScalar x, PixelScalar y, const TCHAR *text)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif

  if (font == NULL)
    return;

  GLTexture *texture = TextCache::get(font, COLOR_BLACK, COLOR_WHITE, text);
  if (texture == NULL)
    return;

  if (background_mode == OPAQUE)
    /* draw the opaque background */
    fill_rectangle(x, y, x + texture->get_width(), y + texture->get_height(),
                   background_color);

  GLEnable scope(GL_TEXTURE_2D);
  texture->bind();
  GLLogicOp logic_op(GL_AND_INVERTED);

  if (background_mode != OPAQUE || background_color != COLOR_BLACK) {
    /* cut out the shape in black */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    texture->draw(x, y);
  }

  if (text_color != COLOR_BLACK) {
    /* draw the text color on top */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    logic_op.set(GL_OR);
    text_color.Set();
    texture->draw(x, y);
  }
}

void
Canvas::text_transparent(PixelScalar x, PixelScalar y, const TCHAR *text)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif

  if (font == NULL)
    return;

  GLTexture *texture = TextCache::get(font, COLOR_BLACK, COLOR_WHITE, text);
  if (texture == NULL)
    return;

  GLEnable scope(GL_TEXTURE_2D);
  texture->bind();
  GLLogicOp logic_op(GL_AND_INVERTED);

  /* cut out the shape in black */
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  texture->draw(x, y);

  if (text_color != COLOR_BLACK) {
    /* draw the text color on top */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    logic_op.set(GL_OR);
    text_color.Set();
    texture->draw(x, y);
  }
}

void
Canvas::text_clipped(PixelScalar x, PixelScalar y, UPixelScalar width,
                     const TCHAR *text)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif

  if (font == NULL)
    return;

  GLTexture *texture = TextCache::get(font, COLOR_BLACK, COLOR_WHITE, text);
  if (texture == NULL)
    return;

  GLEnable scope(GL_TEXTURE_2D);
  texture->bind();
  GLLogicOp logic_op(GL_AND_INVERTED);

  UPixelScalar height = texture->get_height();
  if (texture->get_width() < width)
    width = texture->get_width();

  /* cut out the shape in black */
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  texture->draw(x, y, width, height, 0, 0, width, height);

  if (text_color != COLOR_BLACK) {
    /* draw the text color on top */
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    logic_op.set(GL_OR);
    text_color.Set();
    texture->draw(x, y, width, height, 0, 0, width, height);
  }
}

void
Canvas::stretch(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const GLTexture &texture,
                PixelScalar src_x, PixelScalar src_y,
                UPixelScalar src_width, UPixelScalar src_height)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif

  texture.draw(dest_x, dest_y, dest_width, dest_height,
               src_x, src_y, src_width, src_height);
}

void
Canvas::stretch(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const GLTexture &texture)
{
  stretch(dest_x, dest_y, dest_width, dest_height,
          texture, 0, 0, texture.get_width(), texture.get_height());
}

void
Canvas::copy(PixelScalar dest_x, PixelScalar dest_y,
             UPixelScalar dest_width, UPixelScalar dest_height,
             const Bitmap &src, PixelScalar src_x, PixelScalar src_y)
{
  stretch(dest_x, dest_y, dest_width, dest_height,
          src, src_x, src_y, dest_width, dest_height);
}

void
Canvas::copy(const Bitmap &src)
{
  copy(0, 0, src.get_width(), src.get_height(), src, 0, 0);
}

void
Canvas::stretch_transparent(const Bitmap &src, Color key)
{
  assert(src.defined());

  // XXX
  stretch(src);
}

void
Canvas::invert_stretch_transparent(const Bitmap &src, Color key)
{
  assert(src.defined());

  // XXX
  GLLogicOp invert(GL_COPY_INVERTED);
  stretch(src);
}

void
Canvas::stretch(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const Bitmap &src, PixelScalar src_x, PixelScalar src_y,
                UPixelScalar src_width, UPixelScalar src_height)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif
  assert(src.defined());

  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  GLTexture &texture = *src.native();
  GLEnable scope(GL_TEXTURE_2D);
  texture.bind();
  texture.draw(dest_x, dest_y, dest_width, dest_height,
               src_x, src_y, src_width, src_height);
}

void
Canvas::stretch(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const Bitmap &src)
{
#ifdef HAVE_GLES
  assert(x_offset == OpenGL::translate_x);
  assert(y_offset == OpenGL::translate_y);
#endif
  assert(src.defined());

  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  GLTexture &texture = *src.native();
  GLEnable scope(GL_TEXTURE_2D);
  texture.bind();
  texture.draw(dest_x, dest_y, dest_width, dest_height,
               0, 0, src.get_width(), src.get_height());
}

void
Canvas::copy_or(PixelScalar dest_x, PixelScalar dest_y,
                UPixelScalar dest_width, UPixelScalar dest_height,
                const Bitmap &src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src.defined());

  GLLogicOp logic_op(GL_OR);
  copy(dest_x, dest_y, dest_width, dest_height,
       src, src_x, src_y);
}

void
Canvas::copy_and(PixelScalar dest_x, PixelScalar dest_y,
                 UPixelScalar dest_width, UPixelScalar dest_height,
                 const Bitmap &src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src.defined());

  GLLogicOp logic_op(GL_AND);
  copy(dest_x, dest_y, dest_width, dest_height,
       src, src_x, src_y);
}

void
Canvas::copy_not(PixelScalar dest_x, PixelScalar dest_y,
                 UPixelScalar dest_width, UPixelScalar dest_height,
                 const Bitmap &src, PixelScalar src_x, PixelScalar src_y)
{
  assert(src.defined());

  GLLogicOp logic_op(GL_COPY_INVERTED);
  copy(dest_x, dest_y, dest_width, dest_height,
       src, src_x, src_y);
}

void
Canvas::round_rectangle(PixelScalar left, PixelScalar top,
                        PixelScalar right, PixelScalar bottom,
                        UPixelScalar ellipse_width,
                        UPixelScalar ellipse_height)
{
  UPixelScalar radius = std::min(ellipse_width, ellipse_height) / 2;
  ::RoundRect(*this, left, top, right, bottom, radius);
}
