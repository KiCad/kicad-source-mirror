#ifndef MAIL_TYPE_H_
#define MAIL_TYPE_H_

/**
 * Enum MAIL_T
 * is the set of mail types sendable via KIWAY::ExpressMail() and supplied as
 * the @a aCommand parameter to that function.  Such mail will be received in
 * KIWAY_PLAYER::KiwayMailIn( KIWAY_EXPRESS& aEvent ) and aEvent.Command() will
 * match aCommand to KIWAY::ExpressMail().
 */
enum MAIL_T
{
    MAIL_CROSS_PROBE,               ///< PCB<->SCH, CVPCB->SCH cross-probing.
    MAIL_BACKANNOTATE_FOOTPRINTS,   ///< CVPCB->SCH footprint stuffing at cvpcb termination

};

#endif  // MAIL_TYPE_H_
