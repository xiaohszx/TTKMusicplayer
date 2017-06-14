#ifndef MUSICDOWNLOADQUERYQQCOMMENTSTHREAD_H
#define MUSICDOWNLOADQUERYQQCOMMENTSTHREAD_H

#include "musicdownloadcommentsthread.h"

/*! @brief The class to qq query song comments download data from net.
 * @author Greedysky <greedysky@163.com>
 */
class MUSIC_NETWORK_EXPORT MusicDownLoadQueryQQCommentsThread : public MusicDownLoadCommentsThread
{
    Q_OBJECT
public:
    explicit MusicDownLoadQueryQQCommentsThread(QObject *parent = 0);
    /*!
     * Object contsructor.
     */

    static QString getClassName();
    /*!
     * Get class object name.
     */

    virtual void startToSearch(const QString &name) override;
    /*!
     * Start to Search data from name.
     */
    virtual void startToPage(int offset) override;
    /*!
     * Start to search data from name and type bt paging.
     */

public Q_SLOTS:
    virtual void downLoadFinished() override;
    /*!
     * Download data from net finished.
     */

};

#endif // MUSICDOWNLOADQUERYQQCOMMENTSTHREAD_H