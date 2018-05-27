package com.attackercommunity.acdcbadge;

import android.content.Context;

public interface IBadgeScanner {
    void scan(Context ctx, IBadgeScannerCallback resultCallback);
}
