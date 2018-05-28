package com.attackercommunity.acdcbadge;

import android.content.Context;
import android.graphics.Typeface;

import java.util.HashMap;
import java.util.Map;

public class FontFoundry {
    private static FontFoundry singleton = null;

    private final Map<String, Typeface> mFontMap = new HashMap<>();

    private FontFoundry() {}

    public static FontFoundry Get() {
        if (singleton == null) {
            singleton = new FontFoundry();
        }
        return singleton;
    }

    public Typeface getTypeface(Context ctx, String filename) {
        Typeface res = mFontMap.get(filename);
        if (res == null) {
            res = Typeface.createFromAsset(ctx.getAssets(), "fonts/" + filename);
            mFontMap.put(filename, res);
        }
        return res;
    }
}
