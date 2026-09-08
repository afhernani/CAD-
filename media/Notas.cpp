
// Vértice = tempDimP3 (intersección calculada)
                Point2D vertex = engine_.tempDimP3;
                
                // Calcular radio del arco (distancia vértice → cursor)
                double arcRadius = std::hypot(mousePos.x - vertex.x, mousePos.y - vertex.y);
                
                // Calcular ángulos de las dos líneas respecto al vértice
                double angle1 = std::atan2(engine_.tempDimP2.y - vertex.y, 
                                        engine_.tempDimP2.x - vertex.x);
                double angle2 = std::atan2(mousePos.y - vertex.y, 
                                        mousePos.x - vertex.x);
                
                // Dibujar líneas guía desde el vértice (extendidas)
                sf::Color guideColor(255, 255, 0, 100); // Amarillo semitransparente
                double extLen = arcRadius * 1.5;
                
                sf::Vertex guide1[] = {
                    sf::Vertex(worldToScreen(vertex.x, vertex.y), guideColor),
                    sf::Vertex(worldToScreen(vertex.x + std::cos(angle1) * extLen, 
                                            vertex.y + std::sin(angle1) * extLen), guideColor)
                };
                sf::Vertex guide2[] = {
                    sf::Vertex(worldToScreen(vertex.x, vertex.y), guideColor),
                    sf::Vertex(worldToScreen(vertex.x + std::cos(angle2) * extLen, 
                                            vertex.y + std::sin(angle2) * extLen), guideColor)
                };
                window_.draw(guide1, 2, sf::Lines);
                window_.draw(guide2, 2, sf::Lines);
                
                // Dibujar el arco de cota
                const int numPoints = 64;
                sf::VertexArray arc(sf::LineStrip, numPoints);
                
                // Asegurar que el arco va de angle1 a angle2 en la dirección correcta
                double startAngle = angle1;
                double endAngle = angle2;
                
                // Normalizar para que el arco no dé la vuelta completa
                double diff = endAngle - startAngle;
                while (diff < 0) diff += 2 * PI;
                while (diff >= 2 * PI) diff -= 2 * PI;
                
                double step = diff / (numPoints - 1);
                for (int i = 0; i < numPoints; ++i) {
                    double angle = startAngle + i * step;
                    double px = vertex.x + arcRadius * std::cos(angle);
                    double py = vertex.y + arcRadius * std::sin(angle);
                    arc[i].position = worldToScreen(px, py);
                    arc[i].color = sf::Color(255, 255, 0); // Amarillo
                }
                window_.draw(arc);
                
                // Dibujar flechas en los extremos del arco
                float arrowSize = 6.0f;
                // Flecha en inicio
                sf::Vector2f p1Screen = worldToScreen(
                    vertex.x + arcRadius * std::cos(startAngle),
                    vertex.y + arcRadius * std::sin(startAngle));
                sf::CircleShape arrowStart(arrowSize);
                arrowStart.setFillColor(sf::Color(255, 255, 0));
                arrowStart.setOrigin(arrowSize, arrowSize);
                arrowStart.setPosition(p1Screen);
                window_.draw(arrowStart);
                
                // Flecha en final
                sf::Vector2f p2Screen = worldToScreen(
                    vertex.x + arcRadius * std::cos(endAngle),
                    vertex.y + arcRadius * std::sin(endAngle));
                sf::CircleShape arrowEnd(arrowSize);
                arrowEnd.setFillColor(sf::Color(255, 255, 0));
                arrowEnd.setOrigin(arrowSize, arrowSize);
                arrowEnd.setPosition(p2Screen);
                window_.draw(arrowEnd);
                
                // Mostrar el valor del ángulo junto al cursor
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << engine_.tempDimAngle << "°";
                sf::Text txt;
                txt.setFont(font_);
                txt.setString(toSfString(oss.str()));
                txt.setCharacterSize(14);
                txt.setFillColor(sf::Color(255, 255, 0));
                sf::Vector2f mouseScreen = worldToScreen(mousePos.x, mousePos.y);
                txt.setPosition(mouseScreen.x + 15.f, mouseScreen.y - 25.f);
                window_.draw(txt);

----------

sf::Vertex line1[] = {
            sf::Vertex(worldToScreen(engine_.tempDimP1.x, engine_.tempDimP1.y), feedbackColor),
            sf::Vertex(worldToScreen(engine_.tempDimP2.x, engine_.tempDimP2.y), feedbackColor)
        };
        sf::Vertex line2[] = {
            sf::Vertex(worldToScreen(engine_.tempDimP1.x, engine_.tempDimP1.y), feedbackColor),
            sf::Vertex(worldToScreen(mousePos.x, mousePos.y), feedbackColor)
        };
        window_.draw(line1, 2, sf::Lines);
        window_.draw(line2, 2, sf::Lines);
		
--------------------
app -> App::drawDrawingFeedback

		// --- COTA ANGULAR --- feedback visual más complejo
        else if (engine_.currentMode == Mode::DRAW_DIM_ANGULAR) {
            const double PI = 3.14159265358979323846;
            Point2D mousePos = {currentMouseWorldPos_.x, currentMouseWorldPos_.y};
            
            // Solo mostrar feedback cuando ya tenemos las dos líneas seleccionadas
            // (es decir, cuando el mensaje pide la ubicación del arco)
            if (engine_.statusMessage.find("segunda") != std::string::npos ||
                engine_.statusMessage.find("Segunda") != std::string::npos) {
                
                sf::Vertex line1[] = {
                    sf::Vertex(worldToScreen(engine_.tempDimP1.x, engine_.tempDimP1.y), feedbackColor),
                    sf::Vertex(worldToScreen(engine_.tempDimP2.x, engine_.tempDimP2.y), feedbackColor)
                };
                sf::Vertex line2[] = {
                    sf::Vertex(worldToScreen(engine_.tempDimP1.x, engine_.tempDimP1.y), feedbackColor),
                    sf::Vertex(worldToScreen(mousePos.x, mousePos.y), feedbackColor)
                };
                window_.draw(line1, 2, sf::Lines);
                window_.draw(line2, 2, sf::Lines);
            }
            // PASO 3: Esperando ubicación del arco
            else if (engine_.statusMessage.find("Ubicación") != std::string::npos ||
                    engine_.statusMessage.find("ubicación") != std::string::npos) {
                const double PI = 3.14159265358979323846;
                Point2D vertex = engine_.tempDimP3;
                double arcRadius = std::hypot(mousePos.x - vertex.x, mousePos.y - vertex.y);
                double angle1 = std::atan2(engine_.tempDimP2.y - vertex.y, engine_.tempDimP2.x - vertex.x);
                double angle2 = std::atan2(engine_.tempDimP2_line2.y - vertex.y, engine_.tempDimP2_line2.x - vertex.x);
                // Líneas guía
                sf::Color guideColor(255, 255, 0, 100);
                double extLen = arcRadius * 1.5;
                sf::Vertex guide1[] = {
                    sf::Vertex(worldToScreen(vertex.x, vertex.y), guideColor),
                    sf::Vertex(worldToScreen(vertex.x + std::cos(angle1) * extLen, vertex.y + std::sin(angle1) * extLen), guideColor)
                };
                sf::Vertex guide2[] = {
                    sf::Vertex(worldToScreen(vertex.x, vertex.y), guideColor),
                    sf::Vertex(worldToScreen(vertex.x + std::cos(angle2) * extLen, vertex.y + std::sin(angle2) * extLen), guideColor)
                };
                window_.draw(guide1, 2, sf::Lines);
                window_.draw(guide2, 2, sf::Lines);
                // Arco
                const int numPoints = 64;
                sf::VertexArray arc(sf::LineStrip, numPoints);
                double diff = angle2 - angle1;
                while (diff < 0) diff += 2 * PI;
                while (diff >= 2 * PI) diff -= 2 * PI;
                double step = diff / (numPoints - 1);
                for (int i = 0; i < numPoints; ++i) {
                    double angle = angle1 + i * step;
                    double px = vertex.x + arcRadius * std::cos(angle);
                    double py = vertex.y + arcRadius * std::sin(angle);
                    arc[i].position = worldToScreen(px, py);
                    arc[i].color = sf::Color(255, 255, 0);
                }
                window_.draw(arc);
                // Texto del ángulo
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << engine_.tempDimAngle << "°";
                sf::Text txt;
                txt.setFont(font_);
                txt.setString(toSfString(oss.str()));
                txt.setCharacterSize(14);
                txt.setFillColor(sf::Color::Yellow);
                sf::Vector2f mouseScreen = worldToScreen(mousePos.x, mousePos.y);
                txt.setPosition(mouseScreen.x + 15.f, mouseScreen.y - 25.f);
                window_.draw(txt);
            }
        }



-----------
app.cpp --> drawDimensionText
				else if (dim->type == DimType::ANGULAR) {
                    // Texto en el punto medio del arco
                    const double PI = 3.14159265358979323846;
                    double arcRadius = std::hypot(dim->p3.x - dim->location.x, dim->p3.y - dim->location.y);
                    double angle1 = std::atan2(dim->p1.y - dim->location.y, dim->p1.x - dim->location.x);
                    double angle2 = std::atan2(dim->p2.y - dim->location.y, dim->p2.x - dim->location.x);
                    double diff = angle2 - angle1;
                    while (diff < 0) diff += 2 * PI;
                    while (diff >= 2 * PI) diff -= 2 * PI;
                    double midAngle = angle1 + diff / 2.0;
                    double textRadius = arcRadius * 1.3;
                    textX = dim->location.x + textRadius * std::cos(midAngle);
                    textY = dim->location.y + textRadius * std::sin(midAngle);
                }
				
------------------------
geometry.cpp --> draw(.....)
		else if (type == DimType::ANGULAR) {
            // p1 = punto línea 1, p2 = punto línea 2, location = vértice, p3 = punto en arco
            // Calcular ángulos de las dos líneas respecto al vértice
            double angle1 = std::atan2(p2.y - location.y, p2.x - location.x);
            double angle2 = std::atan2(p3.y - location.y, p3.x - location.x);
            
            // Calcular radio del arco como 30% de la distancia mínima al vértice
            double dist1 = std::hypot(p2.x - location.x, p2.y - location.y);
            double dist2 = std::hypot(p3.x - location.x, p3.y - location.y);
            double arcRadius = std::min(dist1, dist2) * 0.3;
            if (arcRadius < 10.0) arcRadius = 10.0; // Mínimo 10 unidades
            
            // Dibujar el arco
            const int numPoints = 64;
            sf::VertexArray arc(sf::LineStrip, numPoints);
            double diff = angle2 - angle1;
            while (diff < 0) diff += 2 * std::numbers::pi;
            while (diff >= 2 * std::numbers::pi) diff -= 2 * std::numbers::pi;
            double step = diff / (numPoints - 1);
            
            for (int i = 0; i < numPoints; ++i) {
                double angle = angle1 + i * step;
                double px = location.x + arcRadius * std::cos(angle);
                double py = location.y + arcRadius * std::sin(angle);
                arc[i].position = w2s(px, py);
                arc[i].color = color;
            }
            window.draw(arc);
            
            // Líneas de extensión desde el vértice hasta los puntos de las líneas
            sf::Color extColor = color;
            extColor.a = 150;
            sf::Vertex ext1[] = { sf::Vertex(w2s(location.x, location.y), extColor),
                                sf::Vertex(w2s(p1.x, p1.y), extColor) };
            sf::Vertex ext2[] = { sf::Vertex(w2s(location.x, location.y), extColor),
                                sf::Vertex(w2s(p2.x, p2.y), extColor) };
            window.draw(ext1, 2, sf::Lines);
            window.draw(ext2, 2, sf::Lines);
            
            // Flechas en los extremos del arco
            float arrowSize = 4.0f;
            sf::CircleShape arrow1(arrowSize);
            arrow1.setFillColor(color);
            arrow1.setOrigin(arrowSize, arrowSize);
            arrow1.setPosition(w2s(location.x + arcRadius * std::cos(angle1),
                                location.y + arcRadius * std::sin(angle1)));
            window.draw(arrow1);
            
            sf::CircleShape arrow2(arrowSize);
            arrow2.setFillColor(color);
            arrow2.setOrigin(arrowSize, arrowSize);
            arrow2.setPosition(w2s(location.x + arcRadius * std::cos(angle2),
                                location.y + arcRadius * std::sin(angle2)));
            window.draw(arrow2);
        }
---------
engine.cpp > processCoordinate:
			else {
                // PASO 3: Colocar el arco de cota
                auto newDim = std::make_unique<Dimension>();
                newDim->p1 = tempDimP2;          // Punto línea 1 (para dirección)
                newDim->p2 = tempDimP3;          // Punto línea 2 (para dirección)
                newDim->p3 = lastPoint;          // Punto donde hizo clic (para radio del arco)
                newDim->location = tempDimP1;    // Vértice (centro del arco) ← ESTO ESTABA MAL
                newDim->type = DimType::ANGULAR;
                newDim->value = tempDimAngle;
                newDim->layerName = doc.currentLayerName;
                saveState();
                doc.addEntity(std::move(newDim));
                currentMode = Mode::IDLE;
                statusMessage = "Cota angular creada.";
            }

--------------
Dimension::Draw() ->geometry.cpp

else if (type == DimType::ANGULAR) {
    const double PI = 3.14159265358979323846;
    
    // p1 = punto línea 1, p2 = punto línea 2, location = vértice, p3 = punto clic (radio)
    double arcRadius = std::hypot(p3.x - location.x, p3.y - location.y);
    if (arcRadius < 10.0) arcRadius = 10.0;
    
    // Ángulos desde el vértice hacia las dos líneas
    double angle1 = std::atan2(p1.y - location.y, p1.x - location.x);
    double angle2 = std::atan2(p2.y - location.y, p2.x - location.x);
    
    // Dibujar el ARCO
    const int numPoints = 64;
    sf::VertexArray arc(sf::LineStrip, numPoints);
    double diff = angle2 - angle1;
    while (diff < 0) diff += 2 * PI;
    while (diff >= 2 * PI) diff -= 2 * PI;
    double step = diff / (numPoints - 1);
    for (int i = 0; i < numPoints; ++i) {
        double angle = angle1 + i * step;
        double px = location.x + arcRadius * std::cos(angle);
        double py = location.y + arcRadius * std::sin(angle);
        arc[i].position = w2s(px, py);
        arc[i].color = color;
    }
    window.draw(arc);
    
    // Líneas de extensión desde el vértice hasta los extremos del arco
    sf::Color extColor = color;
    extColor.a = 150;
    Point2D ext1End = {location.x + arcRadius * std::cos(angle1), location.y + arcRadius * std::sin(angle1)};
    Point2D ext2End = {location.x + arcRadius * std::cos(angle2), location.y + arcRadius * std::sin(angle2)};
    sf::Vertex ext1[] = { sf::Vertex(w2s(location.x, location.y), extColor), sf::Vertex(w2s(ext1End.x, ext1End.y), extColor) };
    sf::Vertex ext2[] = { sf::Vertex(w2s(location.x, location.y), extColor), sf::Vertex(w2s(ext2End.x, ext2End.y), extColor) };
    window.draw(ext1, 2, sf::Lines);
    window.draw(ext2, 2, sf::Lines);
    
    // Flechas (círculos) en los extremos del arco
    float arrowSize = 4.0f;
    sf::CircleShape arrow1(arrowSize);
    arrow1.setFillColor(color);
    arrow1.setOrigin(arrowSize, arrowSize);
    arrow1.setPosition(w2s(ext1End.x, ext1End.y));
    window.draw(arrow1);
    sf::CircleShape arrow2(arrowSize);
    arrow2.setFillColor(color);
    arrow2.setOrigin(arrowSize, arrowSize);
    arrow2.setPosition(w2s(ext2End.x, ext2End.y));
    window.draw(arrow2);
    
    return; // ← IMPORTANTE: salir aquí para no dibujar línea recta ni flechas triangulares
}

app.cpp app::drawDrewingFeedback()

else if (engine_.statusMessage.find("Ubicación") != std::string::npos ||
                    engine_.statusMessage.find("ubicación") != std::string::npos) {
                const double PI = 3.14159265358979323846;
                Point2D vertex = engine_.tempDimP3;
                double arcRadius = std::hypot(mousePos.x - vertex.x, mousePos.y - vertex.y);
                double angle1 = std::atan2(engine_.tempDimP2.y - vertex.y, engine_.tempDimP2.x - vertex.x);
                double angle2 = std::atan2(engine_.tempDimP2_line2.y - vertex.y, engine_.tempDimP2_line2.x - vertex.x);
                // Líneas guía
                sf::Color guideColor(255, 255, 0, 100);
                double extLen = arcRadius * 1.5;
                sf::Vertex guide1[] = {
                    sf::Vertex(worldToScreen(vertex.x, vertex.y), guideColor),
                    sf::Vertex(worldToScreen(vertex.x + std::cos(angle1) * extLen, vertex.y + std::sin(angle1) * extLen), guideColor)
                };
                sf::Vertex guide2[] = {
                    sf::Vertex(worldToScreen(vertex.x, vertex.y), guideColor),
                    sf::Vertex(worldToScreen(vertex.x + std::cos(angle2) * extLen, vertex.y + std::sin(angle2) * extLen), guideColor)
                };
                window_.draw(guide1, 2, sf::Lines);
                window_.draw(guide2, 2, sf::Lines);
                // Arco
                const int numPoints = 64;
                sf::VertexArray arc(sf::LineStrip, numPoints);
                double diff = angle2 - angle1;
                while (diff < 0) diff += 2 * PI;
                while (diff >= 2 * PI) diff -= 2 * PI;
                double step = diff / (numPoints - 1);
                for (int i = 0; i < numPoints; ++i) {
                    double angle = angle1 + i * step;
                    double px = vertex.x + arcRadius * std::cos(angle);
                    double py = vertex.y + arcRadius * std::sin(angle);
                    arc[i].position = worldToScreen(px, py);
                    arc[i].color = sf::Color(255, 255, 0);
                }
                window_.draw(arc);
                // Texto del ángulo
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << engine_.tempDimAngle << "°";
                sf::Text txt;
                txt.setFont(font_);
                txt.setString(toSfString(oss.str()));
                txt.setCharacterSize(14);
                txt.setFillColor(sf::Color::Yellow);
                sf::Vector2f mouseScreen = worldToScreen(mousePos.x, mousePos.y);
                txt.setPosition(mouseScreen.x + 15.f, mouseScreen.y - 25.f);
                window_.draw(txt);