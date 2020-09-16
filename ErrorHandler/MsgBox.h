#ifndef _NOVA_MSG_BOX_H
#define _NOVA_MSG_BOX_H

#include "ErrorHandler/Components/NodeInfo.h"
#include "ErrorHandler/MessageAnalyzer/ma_utils.h"
#include "ui_MsgBox.h"

#include <list>

namespace novadaq {
namespace errorhandler {

class MsgBox : public QDialog, private Ui::MsgBox
{
	Q_OBJECT

public:
	MsgBox(QString const &title, NodeInfo const &info, QDialog *parent = 0);

private slots:
	void refreshMsgs();

private:
	NodeInfo const &node;
};

}  // end of namespace errorhandler
}  // end of namespace novadaq

#endif
