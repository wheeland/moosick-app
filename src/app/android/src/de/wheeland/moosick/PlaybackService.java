package de.wheeland.moosick;
 
import android.content.Context;
import android.content.Intent;
import org.qtproject.qt5.android.bindings.QtService;
 
public class PlaybackService extends QtService
{
    public static void startMyService(Context ctx) {
        ctx.startService(new Intent(ctx, PlaybackService.class));
    }
}
