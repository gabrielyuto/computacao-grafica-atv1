#include "window.hpp"

void Window::onCreate() {
  auto const *vertexShader{R"gl(#version 300 es
                                   
    layout(location = 0) in vec2 inPosition;
    layout(location = 1) in vec4 inColor;

    out vec4 fragColor;

    void main() { 
      gl_Position = vec4(inPosition, 0, 1);
      fragColor = inColor;
    }
  )gl"};

  auto const *fragmentShader{R"gl(#version 300 es
                                     
    precision mediump float;

    in vec4 fragColor;

    out vec4 outColor;

    void main() { outColor = fragColor; }
  )gl"};

  m_program = abcg::createOpenGLProgram(
      {{.source = vertexShader, .stage = abcg::ShaderStage::Vertex},
       {.source = fragmentShader, .stage = abcg::ShaderStage::Fragment}});

  abcg::glClearColor(0, 0, 0, 1);
  abcg::glClear(GL_COLOR_BUFFER_BIT);

  auto const seed{std::chrono::steady_clock::now().time_since_epoch().count()};
  m_randomEngine.seed(seed);
}

void Window::onPaint() {
  if (m_timer.elapsed() < 1.0 / delay)
    return;
  m_timer.restart();

  setupModel();

  abcg::glViewport(0, 0, m_viewportSize.x, m_viewportSize.y);

  abcg::glUseProgram(m_program);
  abcg::glBindVertexArray(m_VAO);

  abcg::glDrawArrays(type_mode, 0, glDraw_count);

  abcg::glBindVertexArray(0);
  abcg::glUseProgram(0);
}

void Window::onPaintUI() {
  abcg::OpenGLWindow::onPaintUI();

  {
    auto const widgetSize{ImVec2(270, 200)};
    ImGui::SetNextWindowPos(ImVec2(m_viewportSize.x - widgetSize.x - 5,
                                   m_viewportSize.y - widgetSize.y - 5));
    ImGui::SetNextWindowSize(widgetSize);

    auto windowFlags{ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar};
    ImGui::Begin(" ", nullptr, windowFlags);

    if (ImGui::Button("Clear window", ImVec2(260, 30))) {
      abcg::glClear(GL_COLOR_BUFFER_BIT);
    }

    auto colorEditFlags{ImGuiColorEditFlags_NoTooltip |
                        ImGuiColorEditFlags_NoPicker};
    ImGui::PushItemWidth(215);
    ImGui::ColorEdit3("v0", &m_colors.at(0).x, colorEditFlags);
    ImGui::ColorEdit3("v1", &m_colors.at(1).x, colorEditFlags);
    ImGui::ColorEdit3("v2", &m_colors.at(2).x, colorEditFlags);
    ImGui::PopItemWidth();

    static std::size_t currentIndex{};
    std::vector comboItems{"GL_POINTS", "GL_LINES", "GL_LINE_STRIP",
                           "GL_LINE_LOOP", "GL_TRIANGLES"};

    if (ImGui::BeginCombo("Primitivas", comboItems.at(currentIndex))) {
      for (auto index{0U}; index < comboItems.size(); ++index) {
        bool const isSelected{currentIndex == index};
        if (ImGui::Selectable(comboItems.at(index), isSelected)) {
          currentIndex = index;

          abcg::glClear(GL_COLOR_BUFFER_BIT);

          fmt::print("Box Selected.\n");
        }

        switch (currentIndex) {
        case 0:
          type_mode = GL_POINTS;
          break;
        case 1:
          type_mode = GL_LINES;
          break;
        case 2:
          type_mode = GL_LINE_STRIP;
          break;
        case 3:
          type_mode = GL_LINE_LOOP;
          break;
        case 4:
          type_mode = GL_TRIANGLES;
          break;
        case 5:
          type_mode = GL_TRIANGLE_STRIP;
          break;
        case 6:
          type_mode = GL_TRIANGLE_FAN;
          break;
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }

      ImGui::EndCombo();
    }

    if (ImGui::SliderInt("Draw count", &glDraw_count, 3, 20)) {
      abcg::glClear(GL_COLOR_BUFFER_BIT);
    }

    if (ImGui::SliderFloat("Delay", &delay, 1.0f, 100.0f)) {
      abcg::glClear(GL_COLOR_BUFFER_BIT);
    }

    ImGui::End();
  }
}

void Window::onResize(glm::ivec2 const &size) {
  m_viewportSize = size;

  abcg::glClear(GL_COLOR_BUFFER_BIT);
}

void Window::onDestroy() {
  abcg::glDeleteProgram(m_program);
  abcg::glDeleteBuffers(1, &m_VBOPositions);
  abcg::glDeleteBuffers(1, &m_VBOColors);
  abcg::glDeleteVertexArrays(1, &m_VAO);
}

void Window::setupModel() {
  abcg::glDeleteBuffers(1, &m_VBOPositions);
  abcg::glDeleteBuffers(1, &m_VBOColors);
  abcg::glDeleteVertexArrays(1, &m_VAO);

  std::uniform_real_distribution rd(-1.5f, 1.5f);
  std::array<glm::vec2, 3> const positions{
      {{rd(m_randomEngine), rd(m_randomEngine)},
       {rd(m_randomEngine), rd(m_randomEngine)},
       {rd(m_randomEngine), rd(m_randomEngine)}}};

  abcg::glGenBuffers(1, &m_VBOPositions);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBOPositions);
  abcg::glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions.data(),
                     GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  abcg::glGenBuffers(1, &m_VBOColors);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBOColors);
  abcg::glBufferData(GL_ARRAY_BUFFER, sizeof(m_colors), m_colors.data(),
                     GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  auto const positionAttribute{
      abcg::glGetAttribLocation(m_program, "inPosition")};
  auto const colorAttribute{abcg::glGetAttribLocation(m_program, "inColor")};

  abcg::glGenVertexArrays(1, &m_VAO);

  abcg::glBindVertexArray(m_VAO);

  abcg::glEnableVertexAttribArray(positionAttribute);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBOPositions);
  abcg::glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, 0,
                              nullptr);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  abcg::glEnableVertexAttribArray(colorAttribute);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_VBOColors);
  abcg::glVertexAttribPointer(colorAttribute, 4, GL_FLOAT, GL_FALSE, 0,
                              nullptr);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  abcg::glBindVertexArray(0);
}