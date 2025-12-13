#include "scribusproxystyle.h"

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QEvent>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QStatusBar>
#include <QStyleFactory>
#include <QStyleHints>
#include <QStyleOption>
#include <QStyleOptionDockWidget>
#include <QStyleOptionTab>
#include <QStylePainter>
#include <QToolBar>

#include "prefsmanager.h"
#include "scribus.h"
#include "ui/widgets/buttongroup.h"

// https://doc.qt.io/qt-6/qstyle.html#standardPalette
QPalette createLightPalette()
{
	QPalette lightPalette;
	QStyle *fusion = QStyleFactory::create("Fusion");
	if (fusion)
	{
		lightPalette = fusion->standardPalette();
		delete fusion;
	}
	return lightPalette;
}

QPalette createDarkPalette()
{
	QPalette darkPalette;
	darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
	darkPalette.setColor(QPalette::WindowText, Qt::white);
	darkPalette.setColor(QPalette::Base, QColor(42, 42, 42));
	darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
	darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
	darkPalette.setColor(QPalette::ToolTipText, Qt::black);
	darkPalette.setColor(QPalette::Text, Qt::white);
	darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
	darkPalette.setColor(QPalette::ButtonText, Qt::white);
	darkPalette.setColor(QPalette::BrightText, Qt::red);
	darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::HighlightedText, Qt::black);
	darkPalette.setColor(QPalette::Shadow, Qt::black);

	darkPalette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
	darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);

	return darkPalette;
}

ScribusProxyStyle::ScribusProxyStyle(QStyle *style) : QProxyStyle(style) {}
ScribusProxyStyle::ScribusProxyStyle(const QString &key) : QProxyStyle(key) {}

ScribusProxyStyle* ScribusProxyStyle::instance()
{
	return static_cast<ScribusProxyStyle*>(qApp->style());
}

QRect ScribusProxyStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *widget) const
{
	QRect rect = QProxyStyle::subControlRect(cc, opt, sc, widget);

	if (cc == CC_ToolButton)
	{
		const ScToolButton *tb = qobject_cast<const ScToolButton*>(widget);
		if (!tb)
			return rect;

		if (const QStyleOptionToolButton *button = qstyleoption_cast<const QStyleOptionToolButton *>(opt))
		{
			switch (tb->position())
			{
			case ScToolButton::Left:
				rect.adjust(0, 0, rect.width(), 0);
				break;
			case ScToolButton::Center:
				rect.adjust(-rect.width() / 2, 0, rect.width() / 2, 0);
				break;
			case ScToolButton::Right:
				rect.adjust(-rect.width(), 0, 0, 0);
				break;
			default:
				// do nothing
				break;
			}

			return visualRect(button->direction, button->rect, rect);
		}
	}

	return rect;

}

void ScribusProxyStyle::drawControl(ControlElement el, const QStyleOption *opt, QPainter *painter, const QWidget *widget) const
{
	if (el == CE_ToolButtonLabel)
	{
		QStyleOptionToolButton styleOption = *static_cast<const QStyleOptionToolButton *>(opt);
		const ScToolButton *tb = qobject_cast<const ScToolButton*>(widget);

		if (!tb)
			return QProxyStyle::drawControl(el, opt, painter, widget);

		QRect rect = styleOption.rect;
		int t = widget->rect().top() + 4;
		int b = widget->rect().bottom() - 4;
		int r = widget->rect().right();

		switch (tb->position())
		{
		case ScToolButton::Right:
			styleOption.rect.adjust(0, 0, rect.width() / 2, 0);
			break;
		case ScToolButton::Left:
			styleOption.rect.adjust(-rect.width() / 2, 0, 0, 0);
			painter->setPen(opt->palette.color(QPalette::Mid));
			painter->drawLine(r, t, r, b);
			break;
		case ScToolButton::Center:
			styleOption.rect.adjust(-rect.width() / 2, 0, rect.width() / 2, 0);
			painter->setPen(opt->palette.color(QPalette::Mid));
			painter->drawLine(r, t, r, b);
			break;
		default:
			// do nothing
			break;
		}

		return QProxyStyle::drawControl(el, &styleOption, painter, widget);
	}

	return QProxyStyle::drawControl(el, opt, painter, widget);
}

bool ScribusProxyStyle::eventFilter(QObject *object, QEvent *event)
{
	if(object == qApp && event->type() == QEvent::ThemeChange)
	{
		if (PrefsManager::instance().appPrefs.uiPrefs.stylePalette == "auto" && !blockRefresh)
			setApplicationTheme(ScribusProxyStyle::ApplicationTheme::System);

		return true;
	}

	return QObject::eventFilter(object, event);
}

void ScribusProxyStyle::setBaseStyleName(const QString &styleName)
{
	blockRefresh = true;

	QStyle *oldStyle = baseStyle();
	QStyle *newStyle = QStyleFactory::create(styleName);

	// QProxyStyle takes ownership
	setBaseStyle(newStyle);

	if (oldStyle) {
		oldStyle->deleteLater();
	}

	blockRefresh = false;
}


void ScribusProxyStyle::setApplicationTheme(ApplicationTheme theme)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 8, 0))
	blockRefresh = true;

	// For Linux exception see bugreport: https://bugreports.qt.io/browse/QTBUG-132929

	switch (theme)
	{
	case ApplicationTheme::System:
	{
		qApp->styleHints()->unsetColorScheme();
#if (defined Q_OS_LINUX)
		qApp->setPalette(baseStyle()->standardPalette());
#endif
		break;
	}
	case ApplicationTheme::Light:
		qApp->styleHints()->setColorScheme(Qt::ColorScheme::Light);
#if (defined Q_OS_LINUX)
		qApp->setPalette(createLightPalette());
#endif
		break;

	case ApplicationTheme::Dark:
		qApp->styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#if (defined Q_OS_LINUX)
		qApp->setPalette(createDarkPalette());
#endif
		break;
	}

	blockRefresh = false;
#endif
}
