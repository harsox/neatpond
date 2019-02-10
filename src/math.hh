#ifndef _math_h
#define _math_h

#include <cmath>

struct Vector2D {
  float x;
  float y;

  Vector2D(float x, float y): x(x), y(y) { };
  Vector2D(): Vector2D(0.0, 0.0) {};

  void operator += (const Vector2D& other) {
    x += other.x;
    y += other.y;
  }

  Vector2D operator + (const Vector2D& other) const {
    return {x + other.x, y + other.y};
  }

  Vector2D operator - (const Vector2D& other) const {
    return {x - other.x, y - other.y};
  }
};

float modAngle(float angle) {
  float pi2 = M_PI * 2;
  return angle - pi2 * floor(angle / pi2);
}

bool pointCircleCollision(float px, float py, float cx, float cy, float r) {
  if (r == 0) { return false; }
  float dx = cx - px;
  float dy = cy - py;
  return dx * dx + dy * dy <= r * r;
}

bool lineCircleCollide(
  float x1, float y1, float x2, float y2,
  float cx, float cy,
  float radius,
  Vector2D* nearest = nullptr
) {
  // check to see if start or end points lie within circle
  if (pointCircleCollision(x1, y1, cx, cy, radius)) {
    if (nearest != nullptr) {
      nearest->x = x1;
      nearest->y = y1;
    }
    return true;
  }
  if (pointCircleCollision(x2, y2, cx, cy, radius)) {
    if (nearest != nullptr) {
      nearest->x = x1;
      nearest->y = y1;
    }
    return true;
  }

  // vector d
  float dx = x2 - x1;
  float dy = y2 - y1;

  // vector lc
  float lcx = cx - x1;
  float lcy = cy - y1;

  // project lc onto d, resulting in vector p
  float dLen2 = dx * dx + dy * dy; // len2 of d
  float px = dx;
  float py = dy;
  if (dLen2 > 0) {
    float dp = (lcx * dx + lcy * dy) / dLen2;
    px *= dp;
    py *= dp;
  }

  int tmpNearestX = x1 + px;
  int tmpNearestY = y1 + py;

  if (nearest != nullptr) {
    nearest->x = x1 + px;
    nearest->y = y1 + py;
  }

  // len2 of p
  float pLen2 = px * px + py * py;
  bool col = pointCircleCollision(tmpNearestX, tmpNearestY, cx, cy, radius);

  return col && pLen2 <= dLen2 && (px * dx + py * dy) >= 0;
}

#endif
