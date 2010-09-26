/* Torsion - http://torsionim.org/
 * Copyright (C) 2010, John Brooks <special@dereferenced.net>
 *
 * Torsion is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Torsion. If not, see http://www.gnu.org/licenses/
 */

#include "main.h"
#include "ContactRequestDialog.h"
#include "core/ContactsManager.h"
#include "core/IncomingRequestManager.h"
#include "core/NicknameValidator.h"
#include "utils/DateUtil.h"
#include <QBoxLayout>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include <QDateTime>
#include <QToolTip>

ContactRequestDialog::ContactRequestDialog(IncomingContactRequest *r, QWidget *parent)
    : QDialog(parent), request(r)
{
    setWindowTitle(tr("Contact Request"));
    setModal(true);
    setFixedSize(450, 200);

    QBoxLayout *mainLayout = new QVBoxLayout(this);

    /* Introduction */
    QLabel *intro = new QLabel(tr("Someone has requested to add you as a contact"));
    intro->setStyleSheet(QLatin1String("font-weight:bold;"));
    mainLayout->addWidget(intro);

    QTextEdit *message = new QTextEdit;
    message->setReadOnly(true);
    QFont f;
    f.setPointSize(9);
    f.setStyleHint(QFont::SansSerif);
    message->setFont(f);

    message->setText(QString::fromLatin1(
            "<table><tr><td style='color:#808080;'>%1</td><td width='100%' style='font-family:Consolas,\"Courier New\";"
            "font-weight:bold;'>%2</td></tr><tr><td style='color:#808080;padding-right:9px;'>%3</td><td>%4</td></tr><tr>"
            "<td colspan=2><br>%5</td></tr></table>")
            .arg(tr("ID:"))
            .arg(QString::fromLatin1(request->hostname) + QLatin1String("@Torsion"))
            .arg(tr("Date:"))
            .arg(tr("%1 (%2)").arg(timeDifferenceString(request->requestDate()),
                                   request->requestDate().toString(Qt::SystemLocaleShortDate)))
            .arg(Qt::escape(request->message()).replace(QLatin1Char('\n'), QLatin1String("<br>")))
    );

    mainLayout->addWidget(message);

    QBoxLayout *bLayout = new QHBoxLayout;
    mainLayout->addLayout(bLayout);

    /* Nickname */
    QLabel *label = new QLabel(tr("Nickname:"));
    bLayout->addWidget(label);

    m_nickname = new QLineEdit;
#if QT_VERSION >= 0x040700
    m_nickname->setPlaceholderText(tr("Enter a nickname for this contact"));
#endif
    NicknameValidator *nickValidator = new NicknameValidator(m_nickname);
    nickValidator->setWidget(m_nickname);
    nickValidator->setValidateUnique(r->manager->contacts->identity);
    m_nickname->setValidator(nickValidator);
    m_nickname->setFixedWidth(200);
    bLayout->addWidget(m_nickname, 1, Qt::AlignLeft | Qt::AlignVCenter);

    /* Buttons */
    QDialogButtonBox *btns = new QDialogButtonBox(Qt::Horizontal);
    btns->addButton(tr("Accept"), QDialogButtonBox::YesRole);
    btns->addButton(tr("Reject"), QDialogButtonBox::NoRole);
    connect(btns, SIGNAL(accepted()), this, SLOT(accept()));
    connect(btns, SIGNAL(rejected()), this, SLOT(rejectRequest()));

    bLayout->addWidget(btns);

    /* Other */
    connect(&r->manager->contacts->incomingRequests, SIGNAL(requestRemoved(IncomingContactRequest*)),
            SLOT(requestRemoved(IncomingContactRequest*)));
}

void ContactRequestDialog::accept()
{
    if (!m_nickname->hasAcceptableInput())
    {
        m_nickname->setFocus();
        QToolTip::showText(m_nickname->mapToGlobal(QPoint(0,0)),
                           tr("You must enter a valid nickname for this contact"), m_nickname);
        return;
    }

    /* Disconnect from the requestRemoved signal to avoid hitting it here */
    request->manager->contacts->incomingRequests.disconnect(this, SLOT(requestRemoved(IncomingContactRequest*)));

    /* Accept request */
    request->setNickname(m_nickname->text());
    request->accept();

    /* Close dialog */
    QDialog::accept();
}

void ContactRequestDialog::rejectRequest()
{
    /* Disconnect from the requestRemoved signal to avoid hitting it here */
    request->manager->contacts->incomingRequests.disconnect(this, SLOT(requestRemoved(IncomingContactRequest*)));

    request->reject();
    this->done(QDialog::Rejected);
}

void ContactRequestDialog::reject()
{
    this->done(ContactRequestDialog::Cancelled);
}

void ContactRequestDialog::requestRemoved(IncomingContactRequest *r)
{
    if (r != request)
        return;

    /* Request is gone, there is no way for this dialog to stay open (the request will soon be an invalid pointer). */
    reject();
}
