#ifndef _math_h
#define _math_h

struct Vector2D {
  float x;
  float y;

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

bool pointCircleCollision(const Vector2D& point, const Vector2D& circle, float r) {
  if (r == 0) { return false; }
  float dx = circle.x - point.x;
  float dy = circle.y - point.y;
  return dx * dx + dy * dy <= r * r;
}

bool lineCircleCollide(const Vector2D& a, const Vector2D& b, const Vector2D& circle, float radius, Vector2D* nearest = nullptr) {
  // check to see if start or end points lie within circle
  if (pointCircleCollision(a, circle, radius)) {
    if (nearest != nullptr) {
      nearest->x = a.x;
      nearest->y = a.y;
    }
    return true;
  }
  if (pointCircleCollision(b, circle, radius)) {
    if (nearest != nullptr) {
      nearest->x = a.x;
      nearest->y = a.y;
    }
    return true;
  }

  float x1 = a.x;
  float y1 = a.y;
  float x2 = b.x;
  float y2 = b.y;
  float cx = circle.x;
  float cy = circle.y;

  // vector d
  float dx = x2 - x1;
  float dy = y2 - y1;

  // vector lc
  float lcx = cx - x1;
  float lcy = cy - y1;

  // project lc onto d, resulting in vector p
  float dLen2 = dx * dx + dy * dy; //len2 of d
  float px = dx;
  float py = dy;
  if (dLen2 > 0) {
    float dp = (lcx * dx + lcy * dy) / dLen2;
    px *= dp;
    py *= dp;
  }

  Vector2D nearestTmp = {x1 + px, y1 + py};

  if (nearest != nullptr) {
    nearest->x = x1 + px;
    nearest->y = y1 + py;
  }

  // len2 of p
  float pLen2 = px * px + py * py;
  bool col = pointCircleCollision(nearestTmp, circle, radius);

  return col && pLen2 <= dLen2 && (px * dx + py * dy) >= 0;
}

#endif
