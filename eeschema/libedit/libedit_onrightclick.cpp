
#if 0
    case LIB_POLYLINE_T:
        if( item->IsNew() )
        {
            if( ( (LIB_POLYLINE*) item )->GetCornerCount() > 2 )
            {
                msg = AddHotkeyName( _( "Delete" ), g_Libedit_Hotkeys_Descr, HK_DELETE );
                AddMenuItem( PopMenu, ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT,
                             msg, KiBitmap( delete_xpm ) );
            }
        }

        break;

    //===============================================

    case ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT:
        {
            // Delete the last created segment, while creating a polyline draw item
            if( item == NULL )
                break;

            m_canvas->MoveCursorToCrossHair();
            static_cast<LIB_POLYLINE*>( item )->DeleteSegment( GetCrossHairPosition( true ) );
        }
        break;
#endif

