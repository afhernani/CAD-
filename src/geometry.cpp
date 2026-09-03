#include "geometry.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace cad {

    // --- FUNCIÓN AUXILIAR (Privada del archivo) ---
    // Calcula la distancia mínima entre un punto 'p' y el segmento 'a'-'b'
    namespace {
        double distToSegment(const Point2D& p, const Point2D& a, const Point2D& b) {
            double dx = b.x - a.x;
            double dy = b.y - a.y;
            double lenSq = dx * dx + dy * dy;
            
            if (lenSq == 0.0) return std::hypot(p.x - a.x, p.y - a.y);

            double t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / lenSq;
            t = std::max(0.0, std::min(1.0, t)); // Clamp entre 0 y 1

            double projX = a.x + t * dx;
            double projY = a.y + t * dy;

            return std::hypot(p.x - projX, p.y - projY);
        }

        // Calcula el punto reflejado de 'p' respecto al eje definido por 'a' y 'b'
        Point2D reflectPoint(const Point2D& p, const Point2D& a, const Point2D& b) {
            double dx = b.x - a.x;
            double dy = b.y - a.y;
            double lenSq = dx * dx + dy * dy;
            if (lenSq == 0.0) return p; // Eje inválido (punto)

            double px = p.x - a.x;
            double py = p.y - a.y;
            double t = (px * dx + py * dy) / lenSq;

            double projX = a.x + t * dx;
            double projY = a.y + t * dy;

            return { 2.0 * projX - p.x, 2.0 * projY - p.y };
        }
    }

    // --- Line ---
    void Line::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                    const sf::Color& color, float viewScale) const {
        sf::Vertex pts[] = {
            sf::Vertex(w2s(p1.x, p1.y), color),
            sf::Vertex(w2s(p2.x, p2.y), color)
        };
        window.draw(pts, 2, sf::Lines);
    }

    bool Line::isNear(const Point2D& point, double tolerance) const {
        // Distancia punto a segmento
        double dx = p2.x - p1.x;
        double dy = p2.y - p1.y;
        double lenSq = dx * dx + dy * dy;
        
        if (lenSq == 0.0) return std::hypot(point.x - p1.x, point.y - p1.y) <= tolerance;

        double t = ((point.x - p1.x) * dx + (point.y - p1.y) * dy) / lenSq;
        t = std::max(0.0, std::min(1.0, t)); // Clamp entre 0 y 1

        double projX = p1.x + t * dx;
        double projY = p1.y + t * dy;

        double distSq = (point.x - projX) * (point.x - projX) + (point.y - projY) * (point.y - projY);
        return distSq <= (tolerance * tolerance);
    }

    void Line::move(double dx, double dy) {
        p1.x += dx; p1.y += dy;
        p2.x += dx; p2.y += dy;
    }

    std::unique_ptr<Entity> Line::clone() const {
        auto c = std::make_unique<Line>();
        c->p1 = p1; c->p2 = p2;
        c->layerName = layerName;
        return c;
    }
    void Line::rotate(const Point2D& center, double angleDeg) {
        double rad = angleDeg * std::numbers::pi / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);
        auto rot = [&](Point2D& p) {
            double dx = p.x - center.x, dy = p.y - center.y;
            p.x = center.x + dx * cosA - dy * sinA;
            p.y = center.y + dx * sinA + dy * cosA;
        };
        rot(p1); rot(p2);
    }

    void Line::scale(const Point2D& basePoint, double factor) {
        p1.x = basePoint.x + (p1.x - basePoint.x) * factor;
        p1.y = basePoint.y + (p1.y - basePoint.y) * factor;
        p2.x = basePoint.x + (p2.x - basePoint.x) * factor;
        p2.y = basePoint.y + (p2.y - basePoint.y) * factor;
    }

    void Line::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        p1 = reflectPoint(p1, axisP1, axisP2);
        p2 = reflectPoint(p2, axisP1, axisP2);
    }

    // --- Circle ---
    void Circle::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                      const sf::Color& color, float viewScale) const {
        sf::CircleShape shape(static_cast<float>(radius * viewScale));
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineColor(color);
        shape.setOutlineThickness(1.5f);
        shape.setOrigin(static_cast<float>(radius * viewScale), 
                       static_cast<float>(radius * viewScale));
        shape.setPosition(w2s(center.x, center.y));
        window.draw(shape);
    }

    bool Circle::isNear(const Point2D& point, double tolerance) const {
        double dist = std::hypot(point.x - center.x, point.y - center.y);
        // Está cerca si la distancia al centro es aprox el radio (tolerancia)
        return std::abs(dist - radius) <= tolerance;
    }

    void Circle::move(double dx, double dy) {
        center.x += dx; center.y += dy;
    }

    std::unique_ptr<Entity> Circle::clone() const {
        auto c = std::make_unique<Circle>();
        c->center = center; c->radius = radius;
        c->layerName = layerName;
        return c;
    }
    void Circle::rotate(const Point2D& center, double angleDeg) {
        double rad = angleDeg * std::numbers::pi / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);
        double dx = this->center.x - center.x, dy = this->center.y - center.y;
        this->center.x = center.x + dx * cosA - dy * sinA;
        this->center.y = center.y + dx * sinA + dy * cosA;
    }

    void Circle::scale(const Point2D& basePoint, double factor) {
        center.x = basePoint.x + (center.x - basePoint.x) * factor;
        center.y = basePoint.y + (center.y - basePoint.y) * factor;
        radius *= factor;
    }

    void Circle::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        center = reflectPoint(center, axisP1, axisP2);
        // El radio no cambia
    }

    // --- Arc (Arco) ---
    void Arc::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                   const sf::Color& color, float viewScale) const {
        // Usamos 64 puntos para suavidad
        const int numPoints = 64;
        sf::VertexArray va(sf::LineStrip, numPoints);
        
        // Convertir grados a radianes
        double startRad = startAngle * std::numbers::pi / 180.0;
        double endRad = endAngle * std::numbers::pi / 180.0;
        
        // Calcular el paso angular
        // Nota: Si el arco cruza el 0/360, la lógica simple de interpolación lineal 
        // podría dibujar la línea incorrecta. Para simplificar, asumimos barrido directo.
        double step = (endRad - startRad) / (numPoints - 1);

        for (int i = 0; i < numPoints; ++i) {
            double angle = startRad + i * step;
            double px = center.x + radius * std::cos(angle);
            double py = center.y + radius * std::sin(angle);
            
            va[i].position = w2s(px, py);
            va[i].color = color;
        }
        window.draw(va);
    }

    bool Arc::isNear(const Point2D& point, double tolerance) const {
        double dist = std::hypot(point.x - center.x, point.y - center.y);
        // Comprobamos si está sobre el anillo del círculo (radio +/- tolerancia)
        // Nota: Para ser más estrictos podríamos comprobar el ángulo, pero para 
        // selección básica en CAD, comprobar el radio suele ser suficiente y rápido.
        return std::abs(dist - radius) <= tolerance;
    }

    void Arc::move(double dx, double dy) {
        center.x += dx; 
        center.y += dy;
        // El radio y los ángulos no cambian al mover.
    }

    std::unique_ptr<Entity> Arc::clone() const {
        auto c = std::make_unique<Arc>();
        c->center = center; c->radius = radius;
        c->startAngle = startAngle; c->endAngle = endAngle;
        c->layerName = layerName;
        return c;
    }
    void Arc::rotate(const Point2D& center, double angleDeg) {
        double rad = angleDeg * std::numbers::pi / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);
        double dx = this->center.x - center.x, dy = this->center.y - center.y;
        this->center.x = center.x + dx * cosA - dy * sinA;
        this->center.y = center.y + dx * sinA + dy * cosA;
        this->startAngle += angleDeg;
        this->endAngle += angleDeg;
        // Normalizar ángulos al rango [0, 360)
        auto normalize = [](double& ang) {
            while (ang < 0.0) ang += 360.0;
            while (ang >= 360.0) ang -= 360.0;
        };
        normalize(this->startAngle);
        normalize(this->endAngle);
    }

    void Arc::scale(const Point2D& basePoint, double factor) {
        center.x = basePoint.x + (center.x - basePoint.x) * factor;
        center.y = basePoint.y + (center.y - basePoint.y) * factor;
        radius *= factor;
    }

    void Arc::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        center = reflectPoint(center, axisP1, axisP2);
        
        // Calcular ángulo del eje
        double dx = axisP2.x - axisP1.x;
        double dy = axisP2.y - axisP1.y;
        double axisAngle = std::atan2(dy, dx) * 180.0 / std::numbers::pi;
        
        // Reflejar ángulos: alpha' = 2*theta - alpha
        auto reflectAngle = [&](double ang) {
            double newAng = 2.0 * axisAngle - ang;
            while (newAng < 0.0) newAng += 360.0;
            while (newAng >= 360.0) newAng -= 360.0;
            return newAng;
        };
        
        startAngle = reflectAngle(startAngle);
        endAngle = reflectAngle(endAngle);
    }

    // --- Polyline (Polilínea) ---
    void Polyline::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                        const sf::Color& color, float viewScale) const {
        if (points.size() < 2) return;

        sf::VertexArray va(sf::LineStrip, points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            va[i].position = w2s(points[i].x, points[i].y);
            va[i].color = color;
        }
        window.draw(va);
    }

    bool Polyline::isNear(const Point2D& point, double tolerance) const {
        for (size_t i = 0; i + 1 < points.size(); ++i) {
            if (distToSegment(point, points[i], points[i+1]) <= tolerance) {
                return true;
            }
        }
        return false;
    }

    void Polyline::move(double dx, double dy) {
        for (auto& p : points) {
            p.x += dx;
            p.y += dy;
        }
    }

    std::unique_ptr<Entity> Polyline::clone() const {
        auto c = std::make_unique<Polyline>();
        c->points = points;
        c->layerName = layerName;
        return c;
    }
    void Polyline::rotate(const Point2D& center, double angleDeg) {
        double rad = angleDeg * std::numbers::pi / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);
        for (auto& p : points) {
            double dx = p.x - center.x, dy = p.y - center.y;
            p.x = center.x + dx * cosA - dy * sinA;
            p.y = center.y + dx * sinA + dy * cosA;
        }
    }

    void Polyline::scale(const Point2D& basePoint, double factor) {
        for (auto& p : points) {
            p.x = basePoint.x + (p.x - basePoint.x) * factor;
            p.y = basePoint.y + (p.y - basePoint.y) * factor;
        }
    }

    void Polyline::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        for (auto& p : points) {
            p = reflectPoint(p, axisP1, axisP2);
        }
    }

    // --- Polygon (Polígono Regular) ---
    void Polygon::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, const sf::Color& color, float viewScale) const {
        if (sides < 3) return;
        sf::VertexArray va(sf::LineStrip, sides + 1);
        double angleStep = 2.0 * std::numbers::pi / sides;
        for (int i = 0; i <= sides; ++i) {
            // Aplicar el offset de rotación
            double angle = i * angleStep - std::numbers::pi / 2.0 + rotationOffset;
            double px = center.x + radius * std::cos(angle);
            double py = center.y + radius * std::sin(angle);
            va[i].position = w2s(px, py);
            va[i].color = color;
        }
        window.draw(va);
    }

    bool Polygon::isNear(const Point2D& point, double tolerance) const {
        // Un polígono es un bucle cerrado de segmentos.
        // Calculamos los vértices y comprobamos distancia a cada lado.
        double angleStep = 2.0 * std::numbers::pi / sides;
        
        Point2D prev, curr;
        for (int i = 0; i <= sides; ++i) {
            double angle = i * angleStep - std::numbers::pi / 2.0 + rotationOffset;
            curr.x = center.x + radius * std::cos(angle);
            curr.y = center.y + radius * std::sin(angle);
            
            if (i > 0) {
                if (distToSegment(point, prev, curr) <= tolerance) {
                    return true;
                }
            }
            prev = curr;
        }
        return false;
    }

    void Polygon::move(double dx, double dy) {
        center.x += dx; 
        center.y += dy;
        // El radio y los lados no cambian al mover.
    }

    std::unique_ptr<Entity> Polygon::clone() const {
        auto c = std::make_unique<Polygon>();
        c->center = center; c->sides = sides; c->radius = radius;
        c->rotationOffset = rotationOffset; 
        c->layerName = layerName;
        return c;
    }
    void Polygon::rotate(const Point2D& center, double angleDeg) {
        double rad = angleDeg * std::numbers::pi / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);
        double dx = this->center.x - center.x, dy = this->center.y - center.y;
        this->center.x = center.x + dx * cosA - dy * sinA;
        this->center.y = center.y + dx * sinA + dy * cosA;
        this->rotationOffset += rad; // Acumular rotación
    }

    
    void Polygon::scale(const Point2D& basePoint, double factor) {
        center.x = basePoint.x + (center.x - basePoint.x) * factor;
        center.y = basePoint.y + (center.y - basePoint.y) * factor;
        radius *= factor;
    }

    void Polygon::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        center = reflectPoint(center, axisP1, axisP2);
        // El radio y los lados no cambian
    }

        // --- INTERSECCIÓN LÍNEA-LÍNEA ---
    IntersectionResult lineLineIntersection(const Point2D& a1, const Point2D& a2,
                                           const Point2D& b1, const Point2D& b2) {
        IntersectionResult result;
        double dx1 = a2.x - a1.x, dy1 = a2.y - a1.y;
        double dx2 = b2.x - b1.x, dy2 = b2.y - b1.y;
        double denom = dx1 * dy2 - dy1 * dx2;
        
        if (std::abs(denom) < 1e-10) return result; // Paralelas
        
        double t = ((b1.x - a1.x) * dy2 - (b1.y - a1.y) * dx2) / denom;
        double u = ((b1.x - a1.x) * dy1 - (b1.y - a1.y) * dx1) / denom;
        
        result.intersects = true;
        result.point = { a1.x + t * dx1, a1.y + t * dy1 };
        result.param = t;
        return result;
    }

    // --- MÉTODO TRIM PARA LINE ---
    void Line::trim(const Point2D& cutPoint, bool keepStart) {
        double d1 = std::hypot(cutPoint.x - p1.x, cutPoint.y - p1.y);
        double d2 = std::hypot(cutPoint.x - p2.x, cutPoint.y - p2.y);
        
        if (keepStart) {
            // Mantener p1, mover p2 al punto de corte
            p2 = cutPoint;
        } else {
            // Mantener p2, mover p1 al punto de corte
            p1 = cutPoint;
        }
    }

    // --- MÉTODO EXTEND PARA LINE ---
    void Line::extend(const Point2D& borderPoint) {
        // Alargar la línea hasta borderPoint en la dirección del segmento
        double dx = p2.x - p1.x, dy = p2.y - p1.y;
        double len = std::hypot(dx, dy);
        if (len < 1e-10) return;
        
        // Determinar qué extremo está más cerca del borderPoint
        double d1 = std::hypot(borderPoint.x - p1.x, borderPoint.y - p1.y);
        double d2 = std::hypot(borderPoint.x - p2.x, borderPoint.y - p2.y);
        
        if (d1 < d2) {
            // Alargar desde p1
            p1 = borderPoint;
        } else {
            // Alargar desde p2
            p2 = borderPoint;
        }
    }

    // --- Implementaciones de Grips y CopyFrom ---

    // LINE
    std::vector<Point2D> Line::getGripPoints() const { return {p1, p2}; }
    void Line::moveGrip(int index, const Point2D& newPos) {
        if (index == 0) p1 = newPos; else if (index == 1) p2 = newPos;
    }
    void Line::copyFrom(const Entity& src) {
        auto& l = dynamic_cast<const Line&>(src);
        p1 = l.p1; p2 = l.p2; layerName = l.layerName;
    }

    // CIRCLE
    std::vector<Point2D> Circle::getGripPoints() const {
        return {center, {center.x + radius, center.y}};
    }
    void Circle::moveGrip(int index, const Point2D& newPos) {
        if (index == 0) center = newPos;
        else if (index == 1) radius = std::hypot(newPos.x - center.x, newPos.y - center.y);
    }
    void Circle::copyFrom(const Entity& src) {
        auto& c = dynamic_cast<const Circle&>(src);
        center = c.center; radius = c.radius; layerName = c.layerName;
    }

    // ARC
    std::vector<Point2D> Arc::getGripPoints() const {
        const double PI = 3.14159265358979323846;
        double sRad = startAngle * PI / 180.0;
        double eRad = endAngle * PI / 180.0;
        return {
            center,
            {center.x + radius * std::cos(sRad), center.y + radius * std::sin(sRad)},
            {center.x + radius * std::cos(eRad), center.y + radius * std::sin(eRad)}
        };
    }
    void Arc::moveGrip(int index, const Point2D& newPos) {
        const double PI = 3.14159265358979323846;
        if (index == 0) { center = newPos; }
        else if (index == 1) { startAngle = std::atan2(newPos.y - center.y, newPos.x - center.x) * 180.0 / PI; }
        else if (index == 2) { endAngle = std::atan2(newPos.y - center.y, newPos.x - center.x) * 180.0 / PI; }
    }
    void Arc::copyFrom(const Entity& src) {
        auto& a = dynamic_cast<const Arc&>(src);
        center = a.center; radius = a.radius; startAngle = a.startAngle; endAngle = a.endAngle; layerName = a.layerName;
    }

    // POLYLINE
    std::vector<Point2D> Polyline::getGripPoints() const { return points; }
    void Polyline::moveGrip(int index, const Point2D& newPos) {
        if (index >= 0 && index < points.size()) points[index] = newPos;
    }
    void Polyline::copyFrom(const Entity& src) {
        auto& p = dynamic_cast<const Polyline&>(src);
        points = p.points; layerName = p.layerName;
    }

    // POLYGON
    std::vector<Point2D> Polygon::getGripPoints() const {
        const double PI = 3.14159265358979323846;
        std::vector<Point2D> grips;
        
        // Grip en el centro
        grips.push_back(center);
        
        // Grips en los vértices (USANDO rotationOffset)
        double angleStep = 2.0 * PI / sides;
        for (int i = 0; i < sides; ++i) {
            double angle = i * angleStep - PI / 2.0 + rotationOffset;  // ← rotationOffset
            grips.push_back({
                center.x + radius * std::cos(angle),
                center.y + radius * std::sin(angle)
            });
        }
        return grips;
    }
    void Polygon::moveGrip(int index, const Point2D& newPos) {
        if (index == 0) { center = newPos; }
        else { radius = std::hypot(newPos.x - center.x, newPos.y - center.y); } // Mantiene regularidad
    }
    void Polygon::copyFrom(const Entity& src) {
        auto& p = dynamic_cast<const Polygon&>(src);
        center = p.center; sides = p.sides; radius = p.radius; layerName = p.layerName;
    }    

    // --- ELLIPSE ---

    // Función auxiliar para reflejar un punto respecto a un eje definido por dos puntos
    Point2D mirrorPoint(const Point2D& p, const Point2D& axisP1, const Point2D& axisP2) {
        double dx = axisP2.x - axisP1.x;
        double dy = axisP2.y - axisP1.y;
        double lenSq = dx * dx + dy * dy;
        
        // Si el eje es un solo punto (longitud 0), no hay reflexión posible
        if (lenSq == 0.0) return p; 
        
        // Proyección escalar del punto sobre el eje
        double t = ((p.x - axisP1.x) * dx + (p.y - axisP1.y) * dy) / lenSq;
        
        // Coordenadas del "pie" de la perpendicular (punto más cercano en el eje)
        double footX = axisP1.x + t * dx;
        double footY = axisP1.y + t * dy;
        
        // El punto reflejado es simétrico respecto al pie: P' = 2*Pie - P
        return { 2.0 * footX - p.x, 2.0 * footY - p.y };
    }

    void Ellipse::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
                    const sf::Color& color, float viewScale) const {
        const int numPoints = 64;
        sf::VertexArray va(sf::LineStrip, numPoints + 1);
        const double PI = 3.14159265358979323846;
        double angleStep = 2.0 * PI / numPoints;
        
        for (int i = 0; i <= numPoints; ++i) {
            double angle = i * angleStep;
            Point2D pt = getPointOnEllipse(angle);
            va[i].position = w2s(pt.x, pt.y);
            va[i].color = color;
        }
        window.draw(va);
    }

    Point2D Ellipse::getPointOnEllipse(double angle) const {
        // Rotar el ángulo por rotationAngle
        double rotatedAngle = angle + rotationAngle;
        const double PI = 3.14159265358979323846;
        
        // Ecuación paramétrica de la elipse
        double x = center.x + majorRadius * std::cos(rotatedAngle);
        double y = center.y + minorRadius * std::sin(rotatedAngle);
        return {x, y};
    }

    bool Ellipse::isNear(const Point2D& point, double tolerance) const {
        // Comprobar distancia a lo largo de la elipse (muestreo)
        const int numPoints = 64;
        const double PI = 3.14159265358979323846;
        double angleStep = 2.0 * PI / numPoints;
        
        for (int i = 0; i < numPoints; ++i) {
            double angle = i * angleStep;
            Point2D pt = getPointOnEllipse(angle);
            double dist = std::hypot(point.x - pt.x, point.y - pt.y);
            if (dist <= tolerance) {
                return true;
            }
        }
        return false;
    }

    void Ellipse::move(double dx, double dy) {
        center.x += dx;
        center.y += dy;
    }

    void Ellipse::rotate(const Point2D& center, double angleDeg) {
        double rad = angleDeg * std::numbers::pi / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);
        double dx = this->center.x - center.x, dy = this->center.y - center.y;
        this->center.x = center.x + dx * cosA - dy * sinA;
        this->center.y = center.y + dx * sinA + dy * cosA;
        this->rotationAngle += rad;
    }

    void Ellipse::scale(const Point2D& base, double factor) {
        double dx = center.x - base.x, dy = center.y - base.y;
        center.x = base.x + dx * factor;
        center.y = base.y + dy * factor;
        majorRadius *= factor;
        minorRadius *= factor;
    }

    void Ellipse::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        center = mirrorPoint(center, axisP1, axisP2);
        rotationAngle = -rotationAngle; // Invertir rotación
    }

    std::unique_ptr<Entity> Ellipse::clone() const {
        auto c = std::make_unique<Ellipse>();
        c->center = center;
        c->majorRadius = majorRadius;
        c->minorRadius = minorRadius;
        c->rotationAngle = rotationAngle;
        c->layerName = layerName;
        return c;
    }

    void Ellipse::copyFrom(const Entity& src) {
        auto& e = dynamic_cast<const Ellipse&>(src);
        center = e.center;
        majorRadius = e.majorRadius;
        minorRadius = e.minorRadius;
        rotationAngle = e.rotationAngle;
        layerName = e.layerName;
    }

    std::vector<Point2D> Ellipse::getGripPoints() const {
        const double PI = 3.14159265358979323846;
        std::vector<Point2D> grips;
        
        // Centro
        grips.push_back(center);
        
        // 4 puntos en los ejes (0°, 90°, 180°, 270°)
        grips.push_back(getPointOnEllipse(0.0));           // Eje mayor +
        grips.push_back(getPointOnEllipse(PI / 2.0));      // Eje menor +
        grips.push_back(getPointOnEllipse(PI));            // Eje mayor -
        grips.push_back(getPointOnEllipse(3 * PI / 2.0));  // Eje menor -
        
        return grips;
    }

    void Ellipse::moveGrip(int index, const Point2D& newPos) {
        const double PI = 3.14159265358979323846;
        
        if (index == 0) {
            // Mover centro
            double dx = newPos.x - center.x;
            double dy = newPos.y - center.y;
            center.x = newPos.x;
            center.y = newPos.y;
        }
        else if (index == 1) {
            // Eje mayor + (0°)
            double dx = newPos.x - center.x;
            double dy = newPos.y - center.y;
            majorRadius = std::hypot(dx, dy);
            rotationAngle = std::atan2(dy, dx);
        }
        else if (index == 2) {
            // Eje menor + (90°)
            Point2D majorAxisPt = getPointOnEllipse(0.0);
            double dx = newPos.x - center.x;
            double dy = newPos.y - center.y;
            minorRadius = std::hypot(dx, dy);
            // Mantener perpendicularidad
            rotationAngle = std::atan2(majorAxisPt.y - center.y, majorAxisPt.x - center.x);
        }
        else if (index == 3) {
            // Eje mayor - (180°)
            double dx = newPos.x - center.x;
            double dy = newPos.y - center.y;
            majorRadius = std::hypot(dx, dy);
            rotationAngle = std::atan2(dy, dx) - PI;
        }
        else if (index == 4) {
            // Eje menor - (270°)
            Point2D majorAxisPt = getPointOnEllipse(0.0);
            double dx = newPos.x - center.x;
            double dy = newPos.y - center.y;
            minorRadius = std::hypot(dx, dy);
            rotationAngle = std::atan2(majorAxisPt.y - center.y, majorAxisPt.x - center.x);
        }
    }

    // --- Implementaciones de getSnapPoints ---

    std::vector<Point2D> Line::getSnapPoints() const {
        Point2D mid = {(p1.x + p2.x) / 2.0, (p1.y + p2.y) / 2.0};
        return {p1, p2, mid}; // Extremos y Punto Medio
    }

    std::vector<Point2D> Circle::getSnapPoints() const {
        return {center, 
                {center.x + radius, center.y}, 
                {center.x - radius, center.y},
                {center.x, center.y + radius},
                {center.x, center.y - radius}}; // Centro y 4 puntos cardinales
    }

    std::vector<Point2D> Arc::getSnapPoints() const {
        const double PI = 3.14159265358979323846;
        double sRad = startAngle * PI / 180.0;
        double eRad = endAngle * PI / 180.0;
        double midRad = (sRad + eRad) / 2.0;
        // Normalizar ángulo medio si el arco cruza el 0/360
        if (std::abs(eRad - sRad) > PI) midRad += PI;
        
        return {
            center,
            {center.x + radius * std::cos(sRad), center.y + radius * std::sin(sRad)},
            {center.x + radius * std::cos(eRad), center.y + radius * std::sin(eRad)},
            {center.x + radius * std::cos(midRad), center.y + radius * std::sin(midRad)}
        };
    }

    std::vector<Point2D> Polyline::getSnapPoints() const {
        std::vector<Point2D> snaps;
        for (size_t i = 0; i < points.size(); ++i) {
            snaps.push_back(points[i]); // Vértices
            if (i + 1 < points.size()) {
                // Punto medio del segmento
                snaps.push_back({(points[i].x + points[i+1].x)/2.0, (points[i].y + points[i+1].y)/2.0});
            }
        }
        return snaps;
    }

    std::vector<Point2D> Polygon::getSnapPoints() const {
        const double PI = 3.14159265358979323846;
        std::vector<Point2D> snaps = {center};
        double angleStep = 2.0 * PI / sides;
        for (int i = 0; i < sides; ++i) {
            double a1 = i * angleStep - PI / 2.0 + rotationOffset;
            double a2 = (i + 1) * angleStep - PI / 2.0 + rotationOffset;
            double x1 = center.x + radius * std::cos(a1), y1 = center.y + radius * std::sin(a1);
            double x2 = center.x + radius * std::cos(a2), y2 = center.y + radius * std::sin(a2);
            snaps.push_back({x1, y1}); // Vértice
            snaps.push_back({(x1+x2)/2.0, (y1+y2)/2.0}); // Punto medio del lado
        }
        return snaps;
    }

    std::vector<Point2D> Ellipse::getSnapPoints() const {
        std::vector<Point2D> snaps = {center};
        const double PI = 3.14159265358979323846;
        // 4 puntos en los ejes
        snaps.push_back(getPointOnEllipse(0.0));
        snaps.push_back(getPointOnEllipse(PI / 2.0));
        snaps.push_back(getPointOnEllipse(PI));
        snaps.push_back(getPointOnEllipse(3 * PI / 2.0));
        return snaps;
    }
    
    // --- Implementaciones JSON ---

    nlohmann::json Line::toJson() const {
        return {{"type", "Line"}, {"p1", {{"x", p1.x}, {"y", p1.y}}}, {"p2", {{"x", p2.x}, {"y", p2.y}}}, {"layer", layerName}};
    }

    nlohmann::json Circle::toJson() const {
        return {{"type", "Circle"}, {"center", {{"x", center.x}, {"y", center.y}}}, {"radius", radius}, {"layer", layerName}};
    }

    nlohmann::json Arc::toJson() const {
        return {{"type", "Arc"}, {"center", {{"x", center.x}, {"y", center.y}}}, {"radius", radius}, {"startAngle", startAngle}, {"endAngle", endAngle}, {"layer", layerName}};
    }

    nlohmann::json Polyline::toJson() const {
        nlohmann::json pts = nlohmann::json::array();
        for (const auto& p : points) pts.push_back({{"x", p.x}, {"y", p.y}});
        return {{"type", "Polyline"}, {"points", pts}, {"layer", layerName}};
    }

    nlohmann::json Polygon::toJson() const {
        return {{"type", "Polygon"}, {"center", {{"x", center.x}, {"y", center.y}}}, {"sides", sides}, {"radius", radius}, {"rotationOffset", rotationOffset}, {"layer", layerName}};
    }

    nlohmann::json Ellipse::toJson() const {
        return {{"type", "Ellipse"}, {"center", {{"x", center.x}, {"y", center.y}}}, {"majorRadius", majorRadius}, {"minorRadius", minorRadius}, {"rotationAngle", rotationAngle}, {"layer", layerName}};
    }

    // Función factoría para reconstruir entidades desde JSON
    std::unique_ptr<Entity> Entity::fromJson(const nlohmann::json& j) {
        std::string type = j.value("type", "");
        std::string layer = j.value("layer", "0");

        if (type == "Line") {
            auto e = std::make_unique<Line>();
            e->p1 = {j["p1"]["x"].get<double>(), j["p1"]["y"].get<double>()};
            e->p2 = {j["p2"]["x"].get<double>(), j["p2"]["y"].get<double>()};
            e->layerName = layer;
            return e;
        }
        else if (type == "Circle") {
            auto e = std::make_unique<Circle>();
            e->center = {j["center"]["x"].get<double>(), j["center"]["y"].get<double>()};
            e->radius = j["radius"].get<double>();
            e->layerName = layer;
            return e;
        }
        else if (type == "Arc") {
            auto e = std::make_unique<Arc>();
            e->center = {j["center"]["x"].get<double>(), j["center"]["y"].get<double>()};
            e->radius = j["radius"].get<double>();
            e->startAngle = j["startAngle"].get<double>();
            e->endAngle = j["endAngle"].get<double>();
            e->layerName = layer;
            return e;
        }
        else if (type == "Polyline") {
            auto e = std::make_unique<Polyline>();
            for (const auto& pt : j["points"]) {
                e->points.push_back({pt["x"].get<double>(), pt["y"].get<double>()});
            }
            e->layerName = layer;
            return e;
        }
        else if (type == "Polygon") {
            auto e = std::make_unique<Polygon>();
            e->center = {j["center"]["x"].get<double>(), j["center"]["y"].get<double>()};
            e->sides = j["sides"].get<int>();
            e->radius = j["radius"].get<double>();
            e->rotationOffset = j.value("rotationOffset", 0.0);
            e->layerName = layer;
            return e;
        }
        else if (type == "Ellipse") {
            auto e = std::make_unique<Ellipse>();
            e->center = {j["center"]["x"].get<double>(), j["center"]["y"].get<double>()};
            e->majorRadius = j["majorRadius"].get<double>();
            e->minorRadius = j["minorRadius"].get<double>();
            e->rotationAngle = j.value("rotationAngle", 0.0);
            e->layerName = layer;
            return e;
        }
        
        return nullptr; // Tipo desconocido
    }

    // --- DIMENSION ---

    void Dimension::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
                        const sf::Color& color, float viewScale) const {
        // Calcular puntos de extensión y línea de cota
        Point2D ext1, ext2, lineStart, lineEnd;
        
        if (isHorizontal) {
            double y = location.y;
            ext1 = {p1.x, y}; ext2 = {p2.x, y};
            lineStart = {p1.x, y}; lineEnd = {p2.x, y};
        } else {
            double x = location.x;
            ext1 = {x, p1.y}; ext2 = {x, p2.y};
            lineStart = {x, p1.y}; lineEnd = {x, p2.y};
        }

        sf::Color dimColor = color;
        dimColor.a = 200; // Ligeramente transparente para las líneas de extensión

        // Líneas de extensión (desde p1/p2 hasta la línea de cota)
        sf::Vertex extLine1[] = { sf::Vertex(w2s(p1.x, p1.y), dimColor), sf::Vertex(w2s(ext1.x, ext1.y), dimColor) };
        sf::Vertex extLine2[] = { sf::Vertex(w2s(p2.x, p2.y), dimColor), sf::Vertex(w2s(ext2.x, ext2.y), dimColor) };
        window.draw(extLine1, 2, sf::Lines);
        window.draw(extLine2, 2, sf::Lines);

        // Línea de cota
        sf::Vertex dimLine[] = { sf::Vertex(w2s(lineStart.x, lineStart.y), color), sf::Vertex(w2s(lineEnd.x, lineEnd.y), color) };
        window.draw(dimLine, 2, sf::Lines);

        // Ticks (marcas) en los extremos de la línea de cota (líneas perpendiculares pequeñas)
        float tickSize = 5.0f; // Tamaño fijo en píxeles
        sf::Vector2f sPos = w2s(lineStart.x, lineStart.y);
        sf::Vector2f ePos = w2s(lineEnd.x, lineEnd.y);
        
        if (isHorizontal) {
            sf::Vertex tick1[] = { sf::Vertex(sf::Vector2f(sPos.x, sPos.y - tickSize), color), sf::Vertex(sf::Vector2f(sPos.x, sPos.y + tickSize), color) };
            sf::Vertex tick2[] = { sf::Vertex(sf::Vector2f(ePos.x, ePos.y - tickSize), color), sf::Vertex(sf::Vector2f(ePos.x, ePos.y + tickSize), color) };
            window.draw(tick1, 2, sf::Lines);
            window.draw(tick2, 2, sf::Lines);
        } else {
            sf::Vertex tick1[] = { sf::Vertex(sf::Vector2f(sPos.x - tickSize, sPos.y), color), sf::Vertex(sf::Vector2f(sPos.x + tickSize, sPos.y), color) };
            sf::Vertex tick2[] = { sf::Vertex(sf::Vector2f(ePos.x - tickSize, ePos.y), color), sf::Vertex(sf::Vector2f(ePos.x + tickSize, ePos.y), color) };
            window.draw(tick1, 2, sf::Lines);
            window.draw(tick2, 2, sf::Lines);
        }
    }

    bool Dimension::isNear(const Point2D& point, double tolerance) const {
        // Simplificación: comprobar si está cerca de la línea de cota o las de extensión
        Point2D ext1, ext2, lineStart, lineEnd;
        if (isHorizontal) {
            double y = location.y;
            ext1 = {p1.x, y}; ext2 = {p2.x, y};
            lineStart = {p1.x, y}; lineEnd = {p2.x, y};
        } else {
            double x = location.x;
            ext1 = {x, p1.y}; ext2 = {x, p2.y};
            lineStart = {x, p1.y}; lineEnd = {x, p2.y};
        }
        
        double d1 = distToSegment(point, p1, ext1);
        double d2 = distToSegment(point, p2, ext2);
        double d3 = distToSegment(point, lineStart, lineEnd);
        
        return (d1 <= tolerance || d2 <= tolerance || d3 <= tolerance);
    }

    void Dimension::move(double dx, double dy) {
        p1.x += dx; p1.y += dy;
        p2.x += dx; p2.y += dy;
        location.x += dx; location.y += dy;
    }

    void Dimension::rotate(const Point2D& center, double angleDeg) {
        double rad = angleDeg * std::numbers::pi / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);
        
        auto rotatePoint = [&](Point2D& p) {
            double dx = p.x - center.x, dy = p.y - center.y;
            p.x = center.x + dx * cosA - dy * sinA;
            p.y = center.y + dx * sinA + dy * cosA;
        };
        
        rotatePoint(p1); rotatePoint(p2); rotatePoint(location);
        // Recalcular isHorizontal tras rotar (simplificación: mantener el valor actual)
    }

    void Dimension::scale(const Point2D& base, double factor) {
        p1.x = base.x + (p1.x - base.x) * factor; p1.y = base.y + (p1.y - base.y) * factor;
        p2.x = base.x + (p2.x - base.x) * factor; p2.y = base.y + (p2.y - base.y) * factor;
        location.x = base.x + (location.x - base.x) * factor; location.y = base.y + (location.y - base.y) * factor;
        value *= factor;
    }

    void Dimension::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        p1 = mirrorPoint(p1, axisP1, axisP2);
        p2 = mirrorPoint(p2, axisP1, axisP2);
        location = mirrorPoint(location, axisP1, axisP2);
    }

    std::unique_ptr<Entity> Dimension::clone() const {
        auto c = std::make_unique<Dimension>();
        c->p1 = p1; c->p2 = p2; c->location = location;
        c->value = value; c->isHorizontal = isHorizontal;
        c->layerName = layerName;
        return c;
    }

    void Dimension::copyFrom(const Entity& src) {
        auto& d = dynamic_cast<const Dimension&>(src);
        p1 = d.p1; p2 = d.p2; location = d.location;
        value = d.value; isHorizontal = d.isHorizontal;
        layerName = d.layerName;
    }

    std::vector<Point2D> Dimension::getGripPoints() const {
        return {p1, p2, location};
    }

    void Dimension::moveGrip(int index, const Point2D& newPos) {
        if (index == 0) { p1 = newPos; }
        else if (index == 1) { p2 = newPos; }
        else if (index == 2) { location = newPos; }
        
        // Recalcular valor
        if (isHorizontal) value = std::abs(p2.x - p1.x);
        else value = std::abs(p2.y - p1.y);
    }

    std::vector<Point2D> Dimension::getSnapPoints() const {
        return {p1, p2, location};
    }

    nlohmann::json Dimension::toJson() const {
        return {{"type", "Dimension"}, 
                {"p1", {{"x", p1.x}, {"y", p1.y}}}, 
                {"p2", {{"x", p2.x}, {"y", p2.y}}}, 
                {"location", {{"x", location.x}, {"y", location.y}}},
                {"value", value}, {"isHorizontal", isHorizontal}, {"layer", layerName}};
    }

    // Añadir en Entity::fromJson:
    // else if (type == "Dimension") {
    //     auto e = std::make_unique<Dimension>();
    //     e->p1 = {j["p1"]["x"].get<double>(), j["p1"]["y"].get<double>()};
    //     e->p2 = {j["p2"]["x"].get<double>(), j["p2"]["y"].get<double>()};
    //     e->location = {j["location"]["x"].get<double>(), j["location"]["y"].get<double>()};
    //     e->value = j["value"].get<double>();
    //     e->isHorizontal = j["isHorizontal"].get<bool>();
    //     e->layerName = layer;
    //     return e;
    // }


} // namespace cad