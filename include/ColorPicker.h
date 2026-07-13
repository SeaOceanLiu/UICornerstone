#ifndef ColorPickerH
#define ColorPickerH

#include <functional>
#include <vector>
#include <string>
#include "MainWindow.h"
#include "ConstDef.h"
#include "SColor.h"
#include "ControlBase.h"
#include "Panel.h"
#include "Label.h"
#include "EditBox.h"
#include "Slider.h"
#include "Button.h"
#include "Dialog.h"
#include "EventQueue.h"

class ColorPicker : public Panel {
    friend class ColorPickerBuilder;
public:
    using OnColorChangedHandler = std::function<void(shared_ptr<ColorPicker>, const SColor& color)>;

private:
    class PresetCell : public ControlImpl {
    public:
        using OnClickHandler = std::function<void(shared_ptr<Control>)>;
        PresetCell(Control* parent, SRect rect, const SColor& color,
                   float xScale = 1.0f, float yScale = 1.0f);
        void setColor(const SColor& c) { m_color = c; }
        SColor getColor() const { return m_color; }
        void setSelected(bool sel) { m_selected = sel; }
        bool isSelected() const { return m_selected; }
        void setOnClick(OnClickHandler handler) { m_onClick = handler; }
        void draw(void) override;
        bool handleEvent(shared_ptr<Event> event) override;
    private:
        SColor m_color;
        bool m_selected = false;
        OnClickHandler m_onClick;
    };

    SColor m_color;
    SColor m_committedColor;

    shared_ptr<Dialog> m_dialog;
    shared_ptr<Panel>  m_closedSwatch;
    shared_ptr<Label>  m_closedLabel;
    shared_ptr<EditBox> m_hexInput;
    shared_ptr<Control> m_previewSwatch;
    shared_ptr<Slider>  m_sliderR, m_sliderG, m_sliderB, m_sliderA;
    vector<shared_ptr<PresetCell>> m_presetCells;

    vector<SColor> m_presetColors;
    int m_presetCols;
    int m_presetRows;
    bool m_presetGridCreated = false;

    float m_popupWidth;
    float m_popupHeight;

    OnColorChangedHandler m_onColorChanged;
    bool m_isSyncing = false;

    float m_swatchSize = ConstDef::COLORPICKER_SWATCH_SIZE;
    int   m_closedFontSize = ConstDef::COLORPICKER_HEX_FONT_SIZE;
    SColor m_closedTextColor = SColor(219, 219, 219, 255);
    SColor m_popupBGColor = ConstDef::COLORPICKER_POPUP_BG;

    void createClosedStateControls();
    void recreateClosedState();
    SRect computePopupRect();
    void openPopup();
    void closePopup();
    void togglePopup();
    void syncUIFromColor();
    void syncColorFromHex();
    void syncColorFromSliders();
    void createPresetGrid();
    void createHexInput();
    void createSliders();
    void layoutButtons();
    void recreatePopupContent();

    void onPresetClicked(const SColor& color);
    void onHexInputChanged(const string& text);
    void onSliderRChanged(shared_ptr<Slider> s, float val);
    void onSliderGChanged(shared_ptr<Slider> s, float val);
    void onSliderBChanged(shared_ptr<Slider> s, float val);
    void onSliderAChanged(shared_ptr<Slider> s, float val);
    void onOK();
    void onCancel();

public:
    ColorPicker(Control* parent, SRect rect,
                float xScale = 1.0f, float yScale = 1.0f);
    ~ColorPicker();
    void create(void) override;
    bool handleEvent(shared_ptr<Event> event) override;
    void setRect(SRect rect) override;

    void     setColor(const SColor& color);
    void     setColor(const string& hex);
    SColor   getColor() const { return m_color; }
    string   getColorHex() const { return m_color.toHex(false); }

    void setPresetColors(const vector<SColor>& colors);
    void setPresetLayout(int cols, int rows);
    int  getPresetCols() const { return m_presetCols; }
    int  getPresetRows() const { return m_presetRows; }

    bool isPopupVisible() const;

    void setOnColorChanged(OnColorChangedHandler handler) { m_onColorChanged = handler; }

    void setClosedSwatchSize(float size) { m_swatchSize = size; recreateClosedState(); }
    void setClosedFontSize(int size) { m_closedFontSize = size; recreateClosedState(); }
    void setClosedTextColor(SColor color) { m_closedTextColor = color; recreateClosedState(); }
    void setPopupBGColor(SColor color) { m_popupBGColor = color; if (m_dialog) m_dialog->setNormalStateBGColor(color); }
};

class ColorPickerBuilder {
private:
    shared_ptr<ColorPicker> m_colorPicker;
public:
    ColorPickerBuilder(Control* parent, SRect rect,
                       float xScale = 1.0f, float yScale = 1.0f);

    ColorPickerBuilder& setColor(const string& hex);
    ColorPickerBuilder& setPresetColors(const vector<string>& hexList);
    ColorPickerBuilder& setPresetLayout(int cols, int rows);
    ColorPickerBuilder& setOnColorChanged(ColorPicker::OnColorChangedHandler handler);
    ColorPickerBuilder& setClosedSwatchSize(float size);
    ColorPickerBuilder& setClosedFontSize(int size);
    ColorPickerBuilder& setClosedTextColor(const string& hex);
    ColorPickerBuilder& setPopupBGColor(const string& hex);
    ColorPickerBuilder& setBackgroundStateColor(StateColor stateColor);
    ColorPickerBuilder& setBorderStateColor(StateColor stateColor);

    shared_ptr<ColorPicker> build(void);
};

#endif
