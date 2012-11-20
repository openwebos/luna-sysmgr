/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#ifndef VIRTUALKEYBOARD_H
#define VIRTUALKEYBOARD_H

#include <QtPlugin>

#include "IMEDataInterface.h"
#include "InputMethod.h"

// TODO (efigs): write proper doxygen docs. also revise the instructions below

/*! \class VirtualKeyboard
 * Interface for virtual keyboards implemented inside system manager, so that
 * we can have different kinds of keyboards (tablet, phone, pinyin?...)
 * co-exist in the code base without risky overlap.
 *
 * To create a new virtual keyboard:
 *
 * 1. Create a virtual keyboard derived from VirtualKeyboard, which
 *    implements the interfaces of VirtualKeyboard & InputMethod (it derives
 *    from both).
 *
 * 2. Create a factory for it: Derive from VirtualKeyboardFactory and
 *    implement the "create()" method.
 *
 *    Example:
 *
 *    \code
 *    class TabletKeyboardFactory : public VirtualKeyboardFactory
 *    {
 *    public:
 *        TabletKeyboardFactory() : VirtualKeyboardFactory("Tablet Keyboard")
 *        {
 *        }
 *
 *        InputMethod *create(IMEDataInterface *dataInterface)
 *        {
 *            return new TabletKeyboard(dataInterface);
 *        }
 *    };
 *    \endcode
 */
class VirtualKeyboard : public InputMethod
{
public:
    explicit VirtualKeyboard(IMEDataInterface *dataInterface) :
        m_IMEDataInterface(dataInterface) {}

    virtual ~VirtualKeyboard() { m_IMEDataInterface = 0; }

    //! Hides the keyboard.
    void hide() { m_IMEDataInterface->requestHide(); }

    /*! Change the size of the keyboard. Not persisted.
     * \param size -2 is XS, -1 is S, 0 is M, and 1 is L. */
    virtual void requestSize(int size) = 0;

    /*! Change the height of the keyboard. Temporary, not persisted.
     * \param height Height in pixels. */
    virtual void requestHeight(int height) = 0;

    /*! \brief Change the height associated with a size.
     * Not persisted, but will stick through resizes until the setting
     * is changed or sysmgr restarted. */
    virtual void changePresetHeightForSize(int size, int height) = 0;

    /*! For debug purposes, some generic requests can be sent via luna-send
     * commands and processed here */
    virtual bool setBoolOption(const std::string &optionName, bool value) = 0;
    virtual bool setIntOption(const std::string &optionName, int value) = 0;

    /*! Also for debug purposes, a named value can be read from the current
     * keyboard */
    virtual bool getValue(const std::string &name, std::string &outValue) = 0;

    //! Set keyboard & language pair.
    virtual void setKeyboardCombo(const std::string &layoutName,
                                  const std::string &languageName,
                                  bool showLanguageKey) = 0;

    //! Notification that language settings were changed (by the user?)
    virtual void keyboardCombosChanged() = 0;

    /*! Provide a list of keyboard layout that will be shown in the
     * preference app. */
    virtual QList<const char *> getLayoutNameList() = 0;

    /*! \brief Get default language for keyboard layout.
     * Each layout has a default language. Which is it for this layout (which
     * should be in the list above).
     * \return Language string or NULL if the layout is unknown. */
    virtual const char *getLayoutDefaultLanguage(const char *layoutName) = 0;

protected:
    //! desc
    IMEDataInterface *m_IMEDataInterface;
};

/*! \class VirtualKeyboardFactory
 * class description
 */
class VirtualKeyboardFactory
{
public:
    //! description
    typedef enum {
        //! don't even try
        eVirtualKeyboardSupport_NotSupported,
        //! can work, but really not designed for this device
        eVirtualKeyboardSupport_Poor,
        //! supports devices of this size
        eVirtualKeyboardSupport_Preferred_Size,
        /*! supports devices of this size and supports current locale
         * particularly well */
        eVirtualKeyboardSupport_Preferred_SizeAndLocale
    } EVirtualKeyboardSupport;

    virtual ~VirtualKeyboardFactory() {}

    /*! desc
     * \param dataInterface
     * \return  */
    virtual InputMethod *newVirtualKeyboard(IMEDataInterface *dataInterface) = 0;

    /*! desc
     * \return */
    virtual QString name() const = 0;

    /*! desc
     * \param maxWidth
     * \param maxHeight
     * \param dpi
     * \param locale */
    virtual EVirtualKeyboardSupport getSupport(int maxWidth,
                                               int maxHeight,
                                               int dpi,
                                               const char *locale) = 0;
};

Q_DECLARE_INTERFACE(VirtualKeyboardFactory,
                    "com.palm.VirtualKeyboardFactory/1.0")

#define VKB_ENABLE_GLYPH_CACHE 1

// Should always be checked in with 0 for all values...
#define VKB_SHOW_GLYPH_CACHE 0
#define VKB_SHOW_GLYPH_REGIONS 0
#define VKB_FORCE_FPS 0

#endif // VIRTUALKEYBOARD_H
